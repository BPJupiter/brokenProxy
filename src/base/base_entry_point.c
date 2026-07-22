
//////////////
// Globals

global u64           global_update_tick_idx = 0;
global u64           async_threads_count = 0;
global CondVar       async_tick_start_cond_var = {0};
global Mutex         async_tick_start_mutex = {0};
global Mutex         async_tick_stop_mutex = {0};
global bool32        async_loop_again = false;
global bool32        async_loop_again_high_priority = false;
global bool32        global_async_exit = false;
thread_static bool32 is_async_thread = false;

// Defined by target
internal void entry_point(Cmd_Line *cmdline);

internal void main_thread_base_entry_point(int arguments_count, char **arguments)
{
    ThreadNameF("main_thread");
    Temp scratch = scratch_begin(0, 0);

    // set up async thread group info
    async_tick_start_cond_var = cond_var_alloc();
    async_tick_start_mutex = mutex_alloc();
    async_tick_stop_mutex = mutex_alloc();

    // set up telemetry
#if PROFILE_TELEMETRY
    local_persist char tm_data[Megabytes(64)];
    tmLoadLibrary(TM_RELEASE);
    tmSetMaxThreadCount(256);
    tmInitialize(sizeof(tm_data), tm_data);
#endif

    // set up spall
#if PROFILE_SPALL
    spall_profile = spall_init_file_ex("spall_capture", 1, 0);
#endif

    // parse command line
    String8_List command_line_argument_strings = {0};
    {
        for EachIndex(idx, arguments_count) {
            str8_list_push(scratch.arena, &command_line_argument_strings, str8_cstring(arguments[idx]));
        }
    }
    Cmd_Line cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);

    // begin captures
    bool32 capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));
    if (capture) {
        ProfBeginCapture(arguments[0]);
        ProfMsg(BUILD_TITLE);
    }

    // Initialize all included layers
#if defined(HTTP_H) && !defined(HTTP_INIT_MANUAL)
  http_init();
#endif
#if defined(SYMBOL_SERVER_H) && !defined(SMSV_INIT_MANUAL)
  smsv_init();
#endif
#if defined(ARTIFACT_CACHE_H) && !defined(AC_INIT_MANUAL)
  ac_init();
#endif
#if defined(CONTENT_H) && !defined(C_INIT_MANUAL)
  c_init();
#endif
#if defined(FILE_STREAM_H) && !defined(FS_INIT_MANUAL)
  fs_init();
#endif
#if defined(MUTABLE_TEXT_H) && !defined(MTX_INIT_MANUAL)
  mtx_init();
#endif
#if defined(DBG_INFO_H) && !defined(DI_INIT_MANUAL)
  di_init(&cmdline);
#endif
#if defined(DEMON_CORE_H) && !defined(DMN_INIT_MANUAL)
  dmn_init();
#endif
#if defined(DBG_ENGINE_CORE_H) && !defined(D_INIT_MANUAL)
  d_init();
#endif
#if defined(WINDOW_MANAGER_H) && !defined(WM_INIT_MANUAL)
  wm_init();
#endif
#if defined(FONT_PROVIDER_H) && !defined(FP_INIT_MANUAL)
  fp_init();
#endif
#if defined(RENDER_CORE_H) && !defined(R_INIT_MANUAL)
  r_init(&cmdline);
#endif
#if defined(FONT_CACHE_H) && !defined(FNT_INIT_MANUAL)
  fnt_init();
#endif
#if defined(RADDBG_CORE_H) && !defined(RD_INIT_MANUAL)
  rd_init(&cmdline);
#endif

    // launch async threads
#if NEED_ASYNC
    Thread *async_threads = 0;
    u64 lane_broadcast_val = 0;
    {
        u64 num_main_threads = 1;
        u64 num_async_threads = get_system_info()->logical_processor_count;
        u64 num_main_threads_clamped = Min(num_async_threads, num_main_threads);
        num_async_threads -= num_main_threads_clamped;
        String8 num_async_threads_string = cmd_line_string(&cmdline, str8_lit("async_thread_count"));
        if (num_async_threads_string.size != 0) {
            try_u64_from_str8_c_rules(num_async_threads_string, &num_async_threads);
        }
        num_async_threads = Max(1, num_async_threads);
        Barrier barrier = barrier_alloc(num_async_threads);
        Lane_Ctx *lane_ctxs = push_array(scratch.arena, Lane_Ctx, num_async_threads);
        async_threads_count = num_async_threads;
        async_threads = push_array(scratch.arena, Thread, async_threads_count);
        for EachIndex(idx, num_async_threads) {
            lane_ctxs[idx].lane_idx = idx;
            lane_ctxs[idx].lane_count = async_threads_count;
            lane_ctxs[idx].barrier = barrier;
            lane_ctxs[idx].broadcast_memory = &lane_broadcast_val;
            async_threads[idx] = thread_launch(async_thread_entry_point, &lane_ctxs[idx]);
        }
    }
#endif

    // call into entry point
    entry_point(&cmdline);

    // join async threads
#if NEED_ASYNC
    ins_atomic_u32_inc_eval(&global_async_exit);
    ins_atomic_u32_inc_eval(&async_loop_again);
    cond_var_broadcast(async_tick_start_cond_var);
    for EachIndex(idx, async_threads_count) {
        thread_join(async_threads[idx], max_u64);
    }
#endif

    // end captures
    if (capture) {
        ProfEndCapture();
    }

    scratch_end(scratch);
}

internal void supplement_thread_base_entry_point(void (*entry_point)(void *params), void *params)
{
    TCTX *tctx = tctx_alloc();
    tctx_select(tctx);
    entry_point(params);
    tctx_release(tctx);
}

internal u64 update_tick_idx(void)
{
    u64 result = ins_atomic_u64_eval(&global_update_tick_idx);
    return result;
}

internal bool32 update(void)
{
    ProfTick(0);
    ins_atomic_u64_inc_eval(&global_update_tick_idx);
#if defined(FONT_CACHE_H)
    fnt_frame();
#endif
#if OS_FEATURE_GRAPHICAL
    bool32 result = frame();
#else
    bool32 result = false;
#endif
    return result;
}

internal void async_thread_entry_point(void *params)
{
     Lane_Ctx lctx = *(Lane_Ctx *)params;
     lane_ctx(lctx);
     is_async_thread = 1;
     ThreadNameF("async_thread_%I64u", lane_idx());
     for(;;) {
         // rjf: wait for signal if we need, otherwise reset loop signal & continue
         if(lane_idx() == 0) {
             if(!ins_atomic_u32_eval(&async_loop_again)) {
                 MutexScope(async_tick_start_mutex) cond_var_wait(async_tick_start_cond_var, async_tick_start_mutex, now_time_us()+1000000);
             }
             ins_atomic_u32_eval_assign(&async_loop_again, 0);
             ins_atomic_u32_eval_assign(&async_loop_again_high_priority, 0);
         }
         lane_sync();
         
         // rjf: do all ticks for all layers
         ProfScope("async tick")
         {
#if defined(ARTIFACT_CACHE_H)
             ac_async_tick();
#endif
#if defined(CONTENT_H)
             c_async_tick();
#endif
#if defined(FILE_STREAM_H)
             fs_async_tick();
#endif
#if defined(HTTP_H)
             http_async_tick();
#endif
#if defined(SYMBOL_SERVER_H)
             smsv_async_tick();
#endif
#if defined(DBG_INFO_H)
             di_async_tick();
#endif
         }
    
         // rjf: take exit signal; break if set
         lane_sync();
         bool32 need_exit = false;
         if(lane_idx() == 0) {
             need_exit = ins_atomic_u32_eval(&global_async_exit);
         }
         lane_sync_u64(&need_exit, 0);
         if(need_exit) {
             break;
         }
     }
}





#ifndef BASE_THREAD_CONTEXT_H
#define BASE_THREAD_CONTEXT_H

//////////////////
// Lane Context

typedef struct Lane_Ctx Lane_Ctx;
struct Lane_Ctx {
    u64 lane_idx;
    u64 lane_count;
    Barrier barrier;
    u64 *broadcast_memory;
};

///////////////////
// Access Scopes

typedef struct Access_Point Access_Point;
struct Access_Point {
    u64 access_refcount;
    u64 last_time_touched_us;
    u64 last_update_idx_touched;
};

typedef struct Access_Point_Expire_Params Access_Point_Expire_Params;
struct Access_Point_Expire_Params {
    u64 time;
    u64 update_idxs;
};

typedef struct Touch Touch;
struct Touch {
    Touch *next;
    Access_Point *point;
    CondVar cv;
};

typedef struct Access Access;
struct Access {
    Access *next;
    Touch *top_touch;
};

//////////////////////////////////
// Base Per-Thread State Bundle

typedef struct TCTX TCTX;
struct TCTX {
    Arena *arenas[2];

    u8 thread_name[32];
    u64 thread_name_size;

    Lane_Ctx lane_ctx;

    char *file_name;
    u64 line_number;

    Arena *access_arena;
    Access *free_access;
    Touch *free_touch;

    u64 *progress_counter_ptr;
    u64 *progress_target_ptr;
};

//////////////////////////////
// Thread Context Functions

// Thread Context allocation & selection
internal TCTX *tctx_alloc(void);
internal void tctx_release(TCTX *tctx);
internal void tctx_select(TCTX *tctx);
internal TCTX *tctx_selected(void);

// scratch arenas
internal Arena *tctx_get_scratch(Arena **conflicts, u64 count);
#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)

//- rjf: lane metadata
internal Lane_Ctx tctx_set_lane_ctx(Lane_Ctx lane_ctx);
internal void tctx_lane_barrier_wait(void *broadcast_ptr, u64 broadcast_size, u64 broadcast_src_lane_idx);
#define lane_idx() (tctx_selected()->lane_ctx.lane_idx)
#define lane_count() (tctx_selected()->lane_ctx.lane_count)
#define lane_from_task_idx(idx) ((idx)%lane_count())
#define lane_ctx(ctx) tctx_set_lane_ctx((ctx))
#define lane_sync() tctx_lane_barrier_wait(0, 0, 0)
#define lane_sync_u64(ptr, src_lane_idx) tctx_lane_barrier_wait((ptr), sizeof(*(ptr)), (src_lane_idx))
#define lane_range(count) m_range_from_n_idx_m_count(lane_idx(), lane_count(), (count))

//- rjf: thread names
internal void tctx_set_thread_name(String8 name);
internal String8 tctx_get_thread_name(void);

//- rjf: thread source-locations
internal void tctx_write_srcloc(char *file_name, u64 line_number);
internal void tctx_read_srcloc(char **file_name, u64 *line_number);
#define tctx_write_this_srcloc() tctx_write_srcloc(__FILE__, __LINE__)

//- rjf: access scopes
internal Access *access_open(void);
internal void access_close(Access *access);
internal void access_touch(Access *access, Access_Point *pt, CondVar cv);

//- rjf: access points
internal bool32 access_pt_is_expired_(Access_Point *pt, Access_Point_Expire_Params *params);
#define access_pt_is_expired(pt, ...) access_pt_is_expired_((pt), &(Access_Point_Expire_Params){.time = 2000000, .update_idxs = 2, __VA_ARGS__})

//- rjf: progress counters
#define set_progress_ptr(ptr) (tctx_selected()->progress_counter_ptr = (ptr))
#define set_progress_target_ptr(ptr) (tctx_selected()->progress_target_ptr = (ptr))
#define set_progress(val) (tctx_selected()->progress_counter_ptr ? ins_atomic_u64_eval_assign(tctx_selected()->progress_counter_ptr, (val)) : (void)0)
#define add_progress(val) (tctx_selected()->progress_counter_ptr ? ins_atomic_u64_add_eval(tctx_selected()->progress_counter_ptr, (val)) : (void)0)
#define set_progress_target(val) (tctx_selected()->progress_target_ptr ? ins_atomic_u64_eval_assign(tctx_selected()->progress_target_ptr, (val)) : (void)0)


#endif // BASE_THREAD_CONTEXT_H

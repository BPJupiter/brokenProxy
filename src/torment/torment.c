
// command line
global String8 g_stdout_file_name = str8_lit_comp("torment.out");
global String8 g_wdir;
global String8 g_exemplar_dir;
global String8 g_input_data_dir;
global String8 g_out = str8_lit_comp("torment_artifacts");
global bool32  g_verbose;
global bool32  g_redirect_stdout = true;
global bool32  g_stop_on_first_fail_or_crash = true;
global bool32  g_build_only = false;

// tests
global Test_Info *g_sorted_test_infos[ArrayCount(test_infos)] = {0};
global bool32     g_is_first_print;

// invoke
global u64     g_last_exit_code;
global Arena  *g_output_arena;
global String8 g_output;
global String8 g_errors;

// tools


/////////////////////////////


internal String8 t_name_from_test_info(Arena *arena, Test_Info *test_info)
{
    String8 result = str8f(arena, "%S/%S", test_info->layer, test_info->label);
    return result;
}

internal String8_List t_test_layer_from_name(Arena *arena, String8 pattern)
{
    Temp scratch = scratch_begin(&arena, 1);
    String8_List matches = {0};
    for EachIndex(i, test_infos_count) {
        Test_Info *test_info = g_sorted_test_infos[i];
        if (str8_match_wildcard(t_name_from_test_info(scratch.arena, test_info), pattern, 0)) {
            str8_list_push(arena, &matches, test_info->layer);
        }
    }
    scratch_end(scratch);
    return matches;
}

internal bool32 t_write_file_list(String8 name, String8_List data)
{
    Temp scratch = scratch_begin(0, 0);
    String8 path = t_make_file_path(scratch.arena, name);
    bool32 is_written = write_data_list_to_file_path(path, data);
    scratch_end(scratch);
    return is_written;
}

internal bool32 t_write_file(String8 name, String8 data)
{
    String8_Node temp_node = {0};
    temp_node.string = data;

    String8_List temp_list = {0};
    str8_list_push_node(&temp_list, &temp_node);

    return t_write_file_list(name, temp_list);
}

internal String8 t_read_file(Arena *arena, String8 name)
{
    Temp scratch = scratch_begin(&arena, 1);
    String8 path = t_make_file_path(scratch.arena, name);
    String8 data = data_from_file_path(arena, path);
    scratch_end(scratch);
    return data;
}

internal bool32 t_delete_file(String8 name)
{
    Temp scratch = scratch_begin(0, 0);
    String8 path = t_make_file_path(scratch.arena, name);
    bool32 is_deleted = delete_file_at_path(path);
    scratch_end(scratch);
    return is_deleted;
}

internal void t_delete_dir(String8 path)
{
    Temp scratch = scratch_begin(0, 0);
    File_Iter *file_iter = file_iter_begin(scratch.arena, path, 0);
    for (;;) {
        File_Info info = {0};
        if ( ! file_iter_next(scratch.arena, file_iter, &info)) { break; }
        
        if (info.props.flags & FilePropertyFlag_IsFolder) {
            t_delete_dir(str8f(scratch.arena, "%S/%S", path, info.name));
            
            continue;
        }
        
        String8 file_path = str8f(scratch.arena, "%S/%S", path, info.name);
        if ( ! delete_file_at_path(file_path)) {
            fprintf(stderr, "ERROR: unable to delete file %.*s\n", str8_varg(file_path));
        }
    }
    file_iter_end(file_iter);
    
    // TODO: delete directories
    
    scratch_end(scratch);
}

internal String8 t_make_file_path(Arena *arena, String8 name)
{
#if OS_WINDOWS
    return push_str8f(arena, "%S\\%S", g_wdir, name);
#else
    return push_str8f(arena, "%S/%S", g_wdir, name);
#endif
}

internal bool32 t_make_dir(String8 name)
{
    Temp scratch = scratch_begin(0, 0);
    bool32 is_ok = make_directory(t_make_file_path(scratch.arena, name));
    scratch_end(scratch);
    return is_ok;
}

force_inline int t_test_info_is_before(Test_Info **a, Test_Info **b)
{
    String8 layer_a = a[0]->layer;
    String8 layer_b = b[0]->layer;
    int cmp = str8_compar(layer_a, layer_b, 0);
    if (cmp == 0) {
        cmp = u64_compar(&a[0]->decl_line, &b[0]->decl_line);
    }
    return cmp;
}

////////////////////////////////

internal void t_errorf(char *fmt, ...)
{
    Temp scratch = scratch_begin(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 result = push_str8fv(scratch.arena, fmt, args);
    if (g_is_first_print) {
        g_is_first_print = false;
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "%.*s", str8_varg(result));
    va_end(args);
    scratch_end(scratch);
}

////////////////////////////////

internal void t_run_caller(void *raw_ctx)
{
    Temp scratch = scratch_begin(0, 0);

    g_is_first_print = true;

    T_Run_Ctx *ctx = raw_ctx;
    ctx->result.status = TestStatus_Pass;

    String8_List test_out = {0};

    if (ctx->test->skip) {
        ctx->result.status = TestStatus_Skip;
    }
    else {
        Test_Ctx test_ctx = {
            .cmdline = ctx->cmdline,
            .exemplars_path = g_exemplar_dir,
            .artifacts_path = g_wdir,
            .input_data_path = g_input_data_dir,
            .result_out = &ctx->result,
            .test_out = &test_out,
        };
        ctx->test->test_fn(scratch.arena, &test_ctx);
    }

    if (ctx->result.status == TestStatus_Fail || ctx->result.status == TestStatus_Crash) {
        for EachNode(n, String8_Node, test_out.first) {
            t_errorf("%S", n->string);
        }
        if (g_errors.size) {
            t_errorf("%S\n", g_errors);
        }
        if (g_output.size) {
            t_errorf("%S\n", g_output);
        }
    }

    scratch_end(scratch);
}

internal Test_Result t_run(Cmd_Line *cmdline, Test_Info *test, String8 user_data)
{
    T_Run_Ctx ctx = { .test = test, .cmdline = cmdline, .user_data = user_data, .result.status = TestStatus_Fail };
    t_run_caller(&ctx);

    if (ctx.result.status == TestStatus_Fail || ctx.result.status == TestStatus_Crash) {
        if (g_output.size > 0 || g_errors.size > 0) {
            t_errorf("Last captured output:\n");
            if (g_output.size) { t_errorf("%S\n", g_output); }
            if (g_errors.size) { t_errorf("%S\n", g_errors); }
        }
    }

    fflush(stdout);
    fflush(stderr);

    return ctx.result;
}

internal void t_help(void)
{
    fprintf(stderr, "--- Help -------------------------------------------------------\n");
    fprintf(stderr, " %s\n\n", BUILD_TITLE_STRING_LITERAL);
    fprintf(stderr, " Usage: torment [Options] <Input>\n\n");
    fprintf(stderr, " Options:\n");
    fprintf(stderr, "   -list                 Print available test targets\n");
    fprintf(stderr, "   -gui                  Launch debugger with window\n");
    fprintf(stderr, "   -cl:<path>            Override default cl path\n");
    fprintf(stderr, "   -clang:<path>         Override default clang path\n");
    fprintf(stderr, "   -gcc:<path>           Override default gcc path\n");
    fprintf(stderr, "   -linker:<path>        Path to PE/COFF linker\n");
    fprintf(stderr, "   -print_stdout         Print to console stdout and stderr of a run\n");
    fprintf(stderr, "   -out:<path>           Directory path for test outputs (default \"%.*s\")\n", str8_varg(g_out));
    fprintf(stderr, "   -build_only           Build debugger harness without running the tests\n");
    fprintf(stderr, "   -verbose              Enable verbose mode\n");
    fprintf(stderr, "   -help                 Print help menu and exit\n");
    fprintf(stderr, "\nInputs are wildcard expressions. Prefix with ! to skip matches, or + to force-run matches.\n");
    fprintf(stderr, "\nExamples:\n");
    fprintf(stderr, "    torment *                Run all tests\n");
    fprintf(stderr, "    torment bit_array        Run 'bit_array' test\n");
    fprintf(stderr, "    torment !*               Skip all tests\n");
    fprintf(stderr, "    torment * !bit_array     Run all tests but skip 'bit_array'\n");
    fprintf(stderr, "    torment +*               Force-run all tests\n");
}


internal void t_entry_point(Cmd_Line *cmdline)
{
    Temp scratch = scratch_begin(0, 0);
    u64 exit_code = max_u64;

    u64 dashes_size = 9999;
    u8 *dashes = push_array(scratch.arena, u8, dashes_size);
    MemorySet(dashes, '-', dashes_size);

    u64 dots_size = 9999;
    u8 *dots = push_array(scratch.arena, u8, dots_size);
    MemorySet(dots, '.', dots_size);

    char *spaces = "                                                                                      ";

#define PrintHeader(p) fprintf(stderr, "--- %s %.*s\n", p, Max((80-4) - (int)strlen(p), 3), dashes);

    // Handle help

    // Gather tests
    {
        for EachIndex(i, test_infos_count) { g_sorted_test_infos[i] = &test_infos[i]; }
        radsort(g_sorted_test_infos, test_infos_count, (int (*)(void *, void *))t_test_info_is_before);
    }

    // Handle -list
    {
        if (cmd_line_has_flag(cmdline, str8_lit("list"))) {
            PrintHeader("Tests");
            for EachIndex(i, test_infos_count) {
                fprintf(stdout, "  %.*s\n", str8_varg(g_sorted_test_infos[i]->label));
            }
            abort_self(0);
        }
    }

    // Compiler overrides
    {
    }

    // Handle optional -target
    u64_List targets = {0};
    {
        String8_List inputs = {0};

        Cmd_Line_Opt *target_opt = 0;
        if (target_opt == 0) { target_opt = cmd_line_opt_from_string(cmdline, str8_lit("target")); }
        if (target_opt == 0) { target_opt = cmd_line_opt_from_string(cmdline, str8_lit("t"));      }

        // handle explicit target switch
        if (target_opt) {
            str8_list_concat_in_place(&inputs, &target_opt->value_strings);
        }

        // accept inputs from the command line as target tests to run
        str8_list_concat_in_place(&inputs, &cmdline->inputs);

        // no inputs -> print help and exit
        if (inputs.node_count == 0) {
            t_help();
            goto exit;
        }

        Hash_Map hm = {0};
        for EachNode(input_n, String8_Node, inputs.first) {
            String8 t = input_n->string;

            // parse node
            typedef enum { Mode_Default, Mode_Skip, Mode_Force, } Mode;
            Mode mode = Mode_Default;
            if      (str8_match_wildcard(t, str8_lit("+*"), 0)) { mode = Mode_Force; t = str8_skip(t, 1); }
            else if (str8_match_wildcard(t, str8_lit("!*"), 0)) { mode = Mode_Skip;  t = str8_skip(t, 1); }

            if (str8_find_needle(t, 0, str8_lit("/"), 0) >= t.size) {
                t = str8f(scratch.arena, "*/%S", t);
            }

            u64 match_count = 0;

            for EachIndex(test_idx, test_infos_count) {
                Test_Info *test_info = g_sorted_test_infos[test_idx];

                // match test names
                String8 test_name = t_name_from_test_info(scratch.arena, test_info);

                if (str8_match_wildcard(test_name, t, StringMatchFlag_CaseInsensitive)) {
                    // set skip flag
                    switch (mode) {
                        case Mode_Default: break;
                        case Mode_Skip:    test_info->skip = true; break;
                        case Mode_Force:   test_info->skip = false; break;
                    }

                    // append test when not in skipping mode
                    if (!hash_map_search_string_u64(&hm, test_name)) {
                        hash_map_push_string_u64(scratch.arena, &hm, test_name, 1);
                        u64_list_push(scratch.arena, &targets, test_idx);
                    }

                    match_count += 1;
                }
            }

            if (match_count == 0) {
                fprintf(stderr, "WARNING: No matches found for input: %.*s\n", str8_varg(input_n->string));
            }
        }
    }

    g_verbose                     = cmd_line_has_flag(cmdline, str8_lit("verbose"));
    g_redirect_stdout             = !cmd_line_has_flag(cmdline, str8_lit("print_stdout"));
    g_stop_on_first_fail_or_crash = !cmd_line_has_flag(cmdline, str8_lit("keep_going"));
    g_build_only                  = cmd_line_has_flag(cmdline, str8_lit("build_only"));
    g_output_arena                = arena_alloc();

    // default options when running under debugger
#if OS_WINDOWS
    if (!cmd_line_has_flag(cmdline, str8_lit("print_stdout")) && IsDebuggerPresent()) {
        g_redirect_stdout = false;
    }

    // automatically close child processes on exit
    {
        HANDLE job_handle = CreateJobObjectA(0, 0);
        AssertAlways(job_handle != 0);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info = { .BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE };
        AssertAlways(SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info)));
        AssertAlways(AssignProcessToJobObject(job_handle, GetCurrentProcess()));
    }
#endif

    // Handle out
    {
        Cmd_Line_Opt *out_opt = cmd_line_opt_from_string(cmdline, str8_lit("out"));
        if (out_opt) {
            if (out_opt->value_strings.node_count == 1) {
                g_out = out_opt->value_string;
            }
            else {
                fprintf(stderr, "ERROR: -out invalid number of arguments");
            }
        }
    }

    // Make output directory
    make_directory(g_out);
    if (!folder_path_exists(g_out)) {
        fprintf(stderr, "ERROR: unable to create output directory \"%.*s\"\n", str8_varg(g_out));
        goto exit;
    }

    // clean up output from previous run
    delete_file_at_path(g_stdout_file_name);

    //
    // Run Tests
    // 1234567890
    // abcdefghijklmnopqrstuvwxyz
    // ABCDEFGHIJKLMNOPQRSTUVWXYZ
    // ; @ ! == [] {} () ~ * & ^ % $ # - _ "" ? \\ // 
    {
        u64_Array target_indices = u64_array_from_list(scratch.arena, &targets);

        u64 max_label_size = 0;
        u64 max_layer_size = 0;
        for EachIndex(i, target_indices.count) {
            u64 test_idx = target_indices.v[i];
            Test_Info *test_info = g_sorted_test_infos[test_idx];
            max_label_size = Max(max_label_size, test_info->label.size);
            max_layer_size = Max(max_layer_size, test_info->layer.size);
        }

        u64 run_counters[TestStatus_COUNT] = {0};
        u64 max_digit_count  = count_digits_u64(target_indices.count, 10);
        u64 total_time_start = now_time_us();

        typedef struct { u64 target_idx, d; } Slowest;
        Slowest slowest[5] = {0};

        for EachElement(i, slowest) { slowest[i].target_idx = max_u64; }

        u64_List skipped_tests = {0};

        for EachIndex(i, target_indices.count) {
            if (i == 0) { PrintHeader("Tests"); }

            u64 target_idx = target_indices.v[i];
            Test_Info *test = g_sorted_test_infos[target_idx];

            // print run progress
            u64 dots_min = 10;
            u64 dots_count = (max_label_size - test->label.size) + dots_min;
            u64 curr_digit_count = count_digits_u64(i + 1, 10);
            int idx_align_space_count = (int)(max_digit_count - curr_digit_count);
            fprintf(stdout, "[%.*s%llu/%llu] ", idx_align_space_count, spaces, (unsigned long long)i + 1, (unsigned long long)target_indices.count);
            fprintf(stdout, "%.*s %.*s/ %.*s", str8_varg(test->layer), (int)(max_layer_size - test->layer.size), spaces, str8_varg(test->label));
            fprintf(stdout, " %.*s ", (int)dots_count, dots);
            fflush(stdout);
            
            // find exemplar data directory
            {
                String8 binary_dir_path = get_process_info()->binary_path;
                String8 root_dir_path = str8_chop_last_slash(binary_dir_path);
                g_exemplar_dir = str8f(scratch.arena, "%S/local/test_data/exemplars/%S", root_dir_path, test->label);
            }

            // find input data directory
            {
                String8 binary_dir_path = get_process_info()->binary_path;
                String8 root_dir_path = str8_chop_last_slash(binary_dir_path);
                g_input_data_dir = str8f(scratch.arena, "%S/local/test_data/inputs", root_dir_path);
            }

            // setup output directory
            {
                g_wdir = str8f(scratch.arena, "%S/%S", g_out, test->label);
                g_wdir = full_path_from_path(scratch.arena, g_wdir);
            }

            // delete files from the last run in the work directory
            if (folder_path_exists(g_wdir)) {
                t_delete_dir(g_wdir);
            }
            make_directory(g_wdir);

            if (!folder_path_exists(g_out)) {
                fprintf(stderr, "ERROR: Unable to create output directory for test run %.*s\n", str8_varg(g_wdir));
                continue;
            }

            // run test
            u64 run_start_time = now_time_us();
            Test_Result result = t_run(cmdline, test, str8_zero());
            u64 run_end_time = now_time_us();

            // update
            run_counters[result.status] += 1;

            // map status -> color / string
            char *status_name_cstr = 0;
            char *color_cstr = 0;
            switch (result.status) {
                default:
                case TestStatus_Pass: {status_name_cstr = "PASS";  color_cstr = T_GREEN;}break;
                case TestStatus_Fail: {status_name_cstr = "FAIL";  color_cstr = T_RED;}break;
                case TestStatus_Crash:{status_name_cstr = "CRASH"; color_cstr = T_RED;}break;
                case TestStatus_Skip: {status_name_cstr = "SKIP";  color_cstr = T_YELLOW;}break;
            }

            // print run status
            fprintf(stdout, "%s%s" T_RESET, color_cstr, status_name_cstr);

            if (result.status == TestStatus_Pass) {
                u64 d = run_end_time - run_start_time;
                Date_Time t = date_time_from_micro_seconds(d);
                String8 s = string_from_elapsed_time(scratch.arena, t);
                fprintf(stdout, " %.*s", str8_varg(s));

                fflush(stdout);

                u64 insert_idx = max_u64;
                for EachElement(i, slowest) {
                    if (d > slowest[i].d) {
                        insert_idx = i;
                        break;
                    }
                }
                if (insert_idx < ArrayCount(slowest)) {
                    for (u64 i = ArrayCount(slowest) - 1; i > insert_idx; i -= 1) {
                        slowest[i] = slowest[i - 1];
                    }

                    slowest[insert_idx].target_idx = target_idx;
                    slowest[insert_idx].d          = d;
                }
            }
            fprintf(stdout, "\n");

            if (result.status == TestStatus_Fail) {
                fprintf(stdout, "  ERROR: %s:%d: condition: \"%s\"\n", result.fail_file, result.fail_line, result.fail_cond);
            }

            if (result.status == TestStatus_Fail || result.status == TestStatus_Crash) {
                if (g_stop_on_first_fail_or_crash) { goto exit; }
            }

            if (result.status == TestStatus_Skip) {
                u64_list_push(scratch.arena, &skipped_tests, target_idx);
            }
        }

        u64 total_time_end = now_time_us();

        if (target_indices.count > 0 && sum_array_u64(ArrayCount(run_counters), run_counters) > 0) {
            u64     total_time_dt = total_time_end - total_time_start;
            String8 total_time_str = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(total_time_dt));

            fprintf(stderr, "\n");
            PrintHeader("Summary");
            fprintf(stderr, "  Passed   %llu\n", (unsigned long long)run_counters[TestStatus_Pass]);
            fprintf(stderr, "  Failed   %llu\n", (unsigned long long)run_counters[TestStatus_Fail]);
            fprintf(stderr, "  Crashed  %llu\n", (unsigned long long)run_counters[TestStatus_Crash]);
            fprintf(stderr, "  Skipped  %llu\n", (unsigned long long)run_counters[TestStatus_Skip]);
            fprintf(stderr, "  Time     %.*s\n", str8_varg(total_time_str));

            // count slow tests
            u64 slow_count = 0;
            for EachElement(i, slowest) {
                Slowest s = slowest[i];
                if (s.target_idx >= test_infos_count) { break; }
                slow_count += 1;
            }

            if (slow_count > 1) {
                u64 label_max = 0;
                u64 layer_max = 0;
                for EachIndex(i, slow_count) {
                    Slowest s = slowest[i];
                    label_max = Max(g_sorted_test_infos[s.target_idx]->label.size, label_max);
                    layer_max = Max(g_sorted_test_infos[s.target_idx]->layer.size, layer_max);
                }

                fprintf(stderr, "  \nSlow Tests\n");
                for EachIndex(i, slow_count) {
                    Slowest s = slowest[i];
                    if (s.target_idx >= test_infos_count) { break; }

                    Test_Info *test_info    = g_sorted_test_infos[s.target_idx];
                    String8    elapsed_time = string_from_elapsed_time(scratch.arena, date_time_from_micro_seconds(s.d));

                    fprintf(stderr, "    %.*s %.*s/ %.*s %.*s %.*s\n",
                            str8_varg(test_info->layer),
                            (int)(layer_max - test_info->layer.size), spaces,
                            str8_varg(test_info->label),
                            (int)(label_max - test_info->label.size) + 4, dots,
                            str8_varg(elapsed_time));
                }
            }
        }

        exit_code = run_counters[TestStatus_Fail] + run_counters[TestStatus_Crash];
    exit:;
    }

    scratch_end(scratch);
    exit(exit_code);
}
  

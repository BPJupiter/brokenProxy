#ifndef TORMENT_H
#define TORMENT_H

typedef struct T_Run_Ctx T_Run_Ctx;
struct T_Run_Ctx {
    Test_Info *test;
    Cmd_Line *cmdline;
    String8 user_data;
    Test_Result result;
};

////////////////////////////

#define TIMEOUT_US(x)  (x)
#define TIMEOUT_MS(x)  TIMEOUT_US((x) * 1000ull)
#define TIMEOUT_SEC(x) TIMEOUT_MS((x) * 1000ull)

#define ENDT_US(x)  (now_time_us() + (x))
#define ENDT_MS(x)  TIMEOUT_US((x) * 1000ull)
#define ENDT_SEC(x) TIMEOUT_MS((x) * 1000ull)

/////////////////////////////

#define T_RESET  "\x1b[0m"
#define T_RED    "\x1b[31m"
#define T_GREEN  "\x1b[32m"
#define T_YELLOW "\x1b[33m"
#define T_BLUE   "\x1b[34m"

//////////////////////////////


// artifacts directory helpers

internal bool32  t_write_file_list(String8 name, String8_List data);
internal bool32  t_write_file(String8 name, String8 data);
internal String8 t_read_file(Arena *arena, String8 name);
internal bool32  t_delete_file(String8 name);
internal String8 t_make_file_path(Arena *arena, String8 name);

// test runner
internal void        t_run_caller(void *raw_ctx);
internal void        t_run_fail_handler(void *raw_ctx);
internal Test_Result t_run(Cmd_Line *cmdline, Test_Info *test, String8 user_data);

// tools

// Todo: Depricate
internal void t_errorf(char *fmt, ...);
#define T_Ok(c) TestCheck(c)

#endif // TORMENT_H

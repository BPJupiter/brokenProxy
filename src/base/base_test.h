
#ifndef BASE_TEST_H
#define BASE_TEST_H

typedef u32 TestStatus;
enum {
    TestStatus_Fail,
    TestStatus_Crash,
    TestStatus_Pass,
    TestStatus_Skip,
    TestStatus_COUNT
};

typedef struct Test_Result Test_Result;
struct Test_Result {
    TestStatus status;
    char *fail_file;
    int fail_line;
    char *fail_cond;
};

typedef struct Test_Ctx Test_Ctx;
struct Test_Ctx {
    Cmd_Line *cmdline;
    String8 exemplars_path;
    String8 artifacts_path;
    String8 input_data_path;
    Test_Result *result_out;
    String8_List *test_out;
};

#define TEST_FUNCTION_SIG(name) void name(Arena *arena, Test_Ctx *ctx)
#define TEST_FUNCTION_DEF(name) TEST_FUNCTION_SIG(test__##name)
typedef TEST_FUNCTION_SIG(Test_Function_Type);

typedef struct Test_Info Test_Info;
struct Test_Info {
    String8 layer;
    String8 label;
    s64 decl_line;
    bool32 skip;
    Test_Function_Type *test_fn;
};

#if BUILD_TESTS

global u16       test_infos_count = 0;
global Test_Info test_infos[0xffff] = {0};

internal String8 test_build_exe_path(Arena *arena, String8 name);
internal String8 test_input_path    (Arena *arena, Test_Ctx *ctx, String8 name);
internal String8 test_input_exe_path(Arena *arena, Test_Ctx *ctx, String8 name);
internal String8 test_exemplar_path (Arena *arena, Test_Ctx *ctx, String8 name);
internal void base_register_test(char *func_name, Test_Function_Type *fn, char *file_path, int line, int skip);

#define AddTest(name, file_path, line, skip_, ...)                      \
    TEST_FUNCTION_DEF(name);                                            \
    __VA_ARGS__ void add_test__##name(void) { base_register_test(Stringify(name), test__##name, file_path, line, skip_); }

# if COMPILER_MSVC
#  pragma section(".CRT$XCU", read)
#  define DeclareTest(name, skip)                                                                                    \
/* register test          */ AddTest(name, __FILE__, __LINE__, (skip))                                               \
/* alloc function pointer */ __declspec(allocate(".CRT$XCU")) void (*add_test_ptr__##name)(void) = add_test__##name; \
/* do not GC test caller  */ __pragma(comment(linker, "/include:" Stringify(add_test_ptr__##name)))

# elif COMPILER_GCC || COMPILER_CLANG
// clang and gcc allocate memory for the function pointer automatically
#  define DeclareTest(name, skip) AddTest(name, __FILE__, __LINE__, (skip), __attribute__((constructor)))
# else
#  error DeclareTest not defined for this compiler.
# endif
#else
# define DeclareTest(name, skip)
#endif

#define Test(name)        DeclareTest(name, 0) TEST_FUNCTION_DEF(name)
#define SkippedTest(name) DeclareTest(name, 1) TEST_FUNCTION_DEF(name)

#define TestCheck(c) do { if (!(c)) {                                                                                                        \
/* record failed check     */ ctx->result_out[0] = (Test_Result){ .fail_file = __FILE__, .fail_line = __LINE__, .fail_cond = Stringify(c) }; \
/* under debugger? -> trap */ if (debugger_is_attached()) { Trap(); }                                                                        \
/* exit test               */ return;                                                                                                        \
} } while(0)

#define TestSkip() do {                                          \
ctx->result_out[0] = (Test_Result){ .status = TestStatus_Skip }; \
return;                                                          \
} while(0);

#define test_out(string) str8_list_push(arena, ctx->test_out, (string))
#define test_outf(...)   str8_list_pushf(arena, ctx->test_out, __VA_ARGS__);

#endif // BASE_TEST_H

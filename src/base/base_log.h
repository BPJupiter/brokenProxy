// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_LOG_H
#define BASE_LOG_H

////////////////////////////////
//~ rjf: Log Types

typedef enum LogMsgKind {
    LogMsgKind_Info,
    LogMsgKind_UserError,
    LogMsgKind_COUNT
} LogMsgKind;

typedef struct Log_Scope Log_Scope;
struct Log_Scope {
    Log_Scope *next;
    u64 pos;
    String8_List strings[LogMsgKind_COUNT];
};

typedef struct Log_Scope_Result Log_Scope_Result;
struct Log_Scope_Result {
    String8 strings[LogMsgKind_COUNT];
};

typedef struct Log Log;
struct Log {
    Arena *arena;
    Log_Scope *top_scope;
};

////////////////////////////////
// Globals

extern String8 g_logs_folder;

////////////////////////////////
//~ rjf: Log Creation/Selection

internal Log *log_alloc(void);
internal void log_release(Log *log);
internal void log_select(Log *log);

////////////////////////////////
//~ rjf: Log Building

internal void log_msg(LogMsgKind kind, String8 string);
internal void log_msgf(LogMsgKind kind, char *fmt, ...);
#define log_info(s)          log_msg(LogMsgKind_Info, (s))
#define log_infof(...)       log_msgf(LogMsgKind_Info, __VA_ARGS__)
#define log_user_error(s)    log_msg(LogMsgKind_UserError, (s))
#define log_user_errorf(...) log_msgf(LogMsgKind_UserError, __VA_ARGS__)

#define LogInfoNamedBlock(s) DeferLoop(log_infof("%S:\n{\n", (s)), log_infof("}\n"))
#define LogInfoNamedBlockF(...) DeferLoop((log_infof(__VA_ARGS__), log_infof(":\n{\n")), log_infof("}\n"))

////////////////////////////////
//~ rjf: Log Scopes

internal void log_scope_begin(void);
internal Log_Scope_Result log_scope_end(Arena *arena);

#endif // BASE_LOG_H

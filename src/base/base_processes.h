#ifndef BASE_PROCESSES_H
#define BASE_PROCESSES_H

typedef struct Process_Info Process_Info;
struct Process_Info {
    u32 pid;
    bool32 large_pages_allowed;
    String8 binary_file_path;
    String8 binary_path;
    String8 initial_path;
    String8 user_program_config_data_path;
    String8 user_program_cache_data_path;
    String8 user_program_logs_data_path;
    String8_List module_load_paths;
    String8_List environment;
};

typedef struct Process Process;
struct Process {
    u64 u64[1];
};

typedef struct Process_Node Process_Node;
struct Process_Node {
    Process_Node *next;
    Process v;
};

typedef struct Process_List Process_List;
struct Process_List {
    Process_Node *first;
    Process_Node *last;
    u64 count;
};

typedef struct Process_Launch_Params Process_Launch_Params;
struct Process_Launch_Params {
    String8_List cmd_line;
    String8 path;
    String8_List env;
    bool32 inherit_env;
    bool32 debug_subprocess;
    bool32 consoleless;
    File stdout_file;
    File stderr_file;
    File stdin_file;
};

////////////////////////////
// Handle Type Functions

internal Process process_zero(void);
internal bool32 process_match(Process a, Process b);
internal void process_list_push(Arena *arena, Process_List *list, Process p);

///////////////////////////////
// Process Launcher Helpers

internal Process launch_cmd_line(String8 string);
internal Process launch_cmd_linef(char *fmt, ...);

////////////////////////////
// @per_os_impl Aborting

internal void abort_self(u64 exit_code);

////////////////////////////////
// @per_os_impl Process Info

internal Process_Info *get_process_info(void);
internal String8 get_current_path(Arena *arena);
internal u32 get_process_start_time_unix(void);

///////////////////////////////////
// @per_os_impl Child Processes

internal Process process_launch(Process_Launch_Params *params);
internal u64 pid_from_process(Process process);
internal bool32 process_join(Process process, u64 endt_us, u64 *exit_code_out);
internal void process_detach(Process process);
internal bool32 process_kill(Process process);

#endif // BASE_PROCESSES_H

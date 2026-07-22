
#ifndef BASE_COMMAND_LINE_H
#define BASE_COMMAND_LINE_H

///////////////////////////////
// Parsed Command Line Type

typedef struct Cmd_Line_Opt Cmd_Line_Opt;
struct Cmd_Line_Opt {
    Cmd_Line_Opt *next;
    Cmd_Line_Opt *hash_next;
    u64 hash;
    String8 string;
    String8_List value_strings;
    String8 value_string;
};

typedef struct Cmd_Line_Opt_List Cmd_Line_Opt_List;
struct Cmd_Line_Opt_List {
    u64 count;
    Cmd_Line_Opt *first;
    Cmd_Line_Opt *last;
};

typedef struct Cmd_Line Cmd_Line;
struct Cmd_Line {
    String8 exe_name;
    Cmd_Line_Opt_List options;
    String8_List inputs;
    u64 option_table_size;
    Cmd_Line_Opt **option_table;
    u64 argc;
    char **argv;
};

/////////////////////////////////////
// Command Line Parsing Functions

internal Cmd_Line_Opt **cmd_line_slot_from_string(Cmd_Line *cmdline, String8 string);
internal Cmd_Line_Opt  *cmd_line_opt_from_slot   (Cmd_Line_Opt **slot, String8 string);
internal void           cmd_line_push_opt        (Cmd_Line_Opt_List *list, Cmd_Line_Opt *var);
internal Cmd_Line_Opt  *cmd_line_insert_opt      (Arena *arena, Cmd_Line *cmdline, String8 string, String8_List values);
internal Cmd_Line       cmd_line_from_string_list(Arena *arena, String8_List arguments);
internal Cmd_Line_Opt  *cmd_line_opt_from_string (Cmd_Line *cmdline, String8 name);
internal String8_List   cmd_line_strings         (Cmd_Line *cmdline, String8 name);
internal String8        cmd_line_string          (Cmd_Line *cmdline, String8 name);
internal bool32         cmd_line_has_flag        (Cmd_Line *cmdline, String8 name);
internal bool32         cmd_line_has_argument    (Cmd_Line *cmdline, String8 name);

#endif // BASE_COMMAND_LINE_H

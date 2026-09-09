
#ifndef TIDE_CORE_H
#define TIDE_CORE_H

/////////////////
// Cable State

////////////////////////
// Command Kind Types

typedef u32 TI_CmdKindFlags;
enum {
    TI_CmdKindFlag_ListInUI            = (1 << 0),
};
    
///////////////////
// Generated Code

#include "generated/tide.meta.h"

///////////////////
// Command Types

typedef struct TI_Cmd TI_Cmd;
struct TI_Cmd {
    String8 name;
    TI_Regs *regs;
};

typedef struct TI_Cmd_Node TI_Cmd_Node;
struct TI_Cmd_Node {
    TI_Cmd_Node *next;
    TI_Cmd_Node *prev;
    TI_Cmd cmd;
};

typedef struct TI_Cmd_List TI_Cmd_List;
struct TI_Cmd_List {
    TI_Cmd_Node *first;
    TI_Cmd_Node *last;
    u64 count;
};

///////////////////////
// Context Register Types

typedef struct TI_Regs_Node TI_Regs_Node;
struct TI_Regs_Node {
    TI_Regs_Node *next;
    TI_Regs v;
};

/////////////////////
// Per-Window State

typedef struct TI_Window_State TI_Window_State;
struct TI_Window_State {
    // links & metadata
    TI_Window_State *order_next;
    TI_Window_State *order_prev;
    TI_Window_State *hash_next;
    TI_Window_State *hash_prev;
    CFG_ID cfg_id;
    u64 frames_alive;
    u64 last_frame_index_touched;
    
    // top-level info & handles
    Arena *arena;
    WM_Window os;
    R_Handle r;
    UI_State *ui;
    f32 last_dpi;
    Rng2f32 last_window_rect;

    // theme (recomputed each frame)
    UI_Theme *theme;

    // error state
    u8 error_buffer[512];
    u64 error_string_size;
    f32 error_t;

    // per-frame ui events state
    UI_Event_List ui_events;

    // per-frame drawing state
    DR_Bucket *draw_bucket;
};

typedef struct TI_Window_State_Slot TI_Window_State_Slot;
struct TI_Window_State_Slot {
    TI_Window_State *first;
    TI_Window_State *last;
};

////////////////////////////////////
// Main Per-Process Graphical State

typedef struct TI_State TI_State;
struct TI_State {
    // basics
    Arena *arena;
    bool32 quit;
    s32 frame_depth;

    // animation rates
    f32 catchall_animation_rate;
    f32 menu_animation_rate;
    f32 menu_animation_rate__slow;
    f32 entity_alive_animation_rate;
    f32 rich_hover_animation_rate;
    f32 scrolling_animation_rate;
    f32 tooltip_animation_rate;

    // log
    Log *log;
    String8 log_path;

    // frame history info
    u64 frame_index;
    Arena *frame_arenas[2];
    u64 frame_time_us_history[64];
    u64 num_frames_requested;
    f64 time_in_seconds;
    u64 time_in_us;

    // frame parameters
    f32 frame_dt;
    Access *frame_access;
    String8 last_window_title;

    // Registers stack
    TI_Regs_Node base_regs;
    TI_Regs_Node *top_regs;

    // commands
    Arena *cmds_arenas[2];
    TI_Cmd_List cmds[2];
    u64 cmds_gen;
    Arena *cmd_output_arena;
    String8_List cmd_outputs;

    // cfg state
    CFG_State *cfg;

    // window state cache
    u64 window_state_slots_count;
    TI_Window_State_Slot *window_state_slots;
    TI_Window_State *free_window_state;
    CFG_ID last_focused_window;
    TI_Window_State *first_window_state;
    TI_Window_State *last_window_state;
    CFG_ID window_state_last_accessed_id;
    TI_Window_State *window_state_last_accessed;
};

//////////////
// Globals

read_only global TI_Cmd_Kind_Info ti_nil_cmd_kind_info = {0};

read_only global TI_Window_State ti_nil_window_state = {
    &ti_nil_window_state,
    &ti_nil_window_state,
    &ti_nil_window_state,
    &ti_nil_window_state,
};

global TI_State *ti_state = 0;

// Dev flags
global bool32 DEV_draw_diag_line_test = false;
global bool32 DEV_draw_3D_test        = true;
global bool32 DEV_button_test         = false;
global bool32 DEV_draw_ui_text_pos    = false;
global bool32 DEV_draw_ui_focus_debug = false;
global bool32 DEV_draw_ui_box_heatmap = false;

/////////////////////////////
// Registers Type Functions

internal void ti_regs_copy_contents(Arena *arena, TI_Regs *dst, TI_Regs *src);
internal TI_Regs *ti_regs_copy(Arena *arena, TI_Regs *src);

///////////////////////////
// Commands type functions

internal void ti_cmd_list_push_new(Arena *arena, TI_Cmd_List *cmds, String8 name, TI_Regs *regs);

////////////////////
// Config Functions

/*
internal bool32 bp_cfg_is_project_filtered(CFG_Node *cfg);
*/

internal Vec4f32 bp_hsva_from_cfg(CFG_Node *cfg);
internal Vec4f32 bp_color_from_cfg(CFG_Node *cfg);

/*
internal bool32 bp_disabled_from_cfg(CFG_Node *cfg);
*/
internal String8 bp_name_from_cfg(CFG_Node *cfg);
internal String8 bp_label_from_cfg(CFG_Node *cfg);
internal String8 bp_path_from_cfg(CFG_Node *cfg);

/*
internal String8 bp_default_setting_from_names(String8 schema_name, String8 setting_name);


internal String8 bp_setting_from_name(String8 name);
internal bool32 bp_setting_bool32_from_name(String8 name);
internal u64 bp_setting_u64_from_name(String8 name);
internal f32 bp_setting_f32_from_name(String8 name);
*/

internal CFG_Node *bp_immediate_cfg_from_key(String8 string);
internal CFG_Node *bp_immediate_cfg_from_keyf(char *fmt, ...);

///////////////////
// Window Functions

internal String8 ti_push_window_title(Arena *arena);
internal CFG_Node *ti_window_from_cfg(CFG_Node *cfg);
internal TI_Window_State *ti_window_state_from_cfg(CFG_Node *cfg);
internal TI_Window_State *ti_window_state_from_os_handle(WM_Window os);
internal void ti_window_frame(void);

///////////////////////
// Colors, Fonts, Config

// colors

// fonts
internal f32 ti_font_size(void);

////////////////////////////
// Continuous Frame Requests

internal void ti_request_frame(void);

///////////////////////
// Main State Accessors

// per-frame arena
internal Arena *ti_frame_arena(void);

////////////////
// Registers

#define ti_regs() (&ti_state->top_regs->v)
#define ti_base_regs() (&ti_state->base_regs.v)
internal TI_Regs *ti_push_regs_(TI_Regs *regs);
#define ti_push_regs(...) ti_push_regs_(&(TI_Regs){ti_regs_lit_init_top __VA_ARGS__})
internal TI_Regs *ti_pop_regs(void);
#define TI_RegsScope(...) DeferLoop(ti_push_regs(__VA_ARGS__), ti_pop_regs())

//////////////
// Commands

// name -> info
internal TI_CmdKind ti_cmd_kind_from_string(String8 string);
internal TI_Cmd_Kind_Info *ti_cmd_kind_info_from_string(String8 string);

// pushing
internal void ti_push_cmd(String8 name, TI_Regs *regs);
#define ti_cmd(kind, ...) ti_push_cmd(ti_cmd_kind_info_table[kind].string, &(TI_Regs){ti_regs_lit_init_top __VA_ARGS__})

// iterating
internal bool32 ti_next_cmd(TI_Cmd **cmd);
/*
internal bool32 ti_next_view_cmd(TI_Cmd **cmd);
*/

//////////////////////////////
// Main Layer Top-Level Calls

internal void ti_init(Cmd_Line *cmdline);
internal void ti_frame(void);

#endif // TIDE_CORE_H

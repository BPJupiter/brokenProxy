
#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

///////////////////////////
// Graphics System Info

typedef struct WM_System_Info WM_System_Info;
struct WM_System_Info {
    f32 double_click_time;
    f32 caret_blink_time;
    f32 default_refresh_rate;
};

///////////////////
// Window Types

typedef u32 WM_WindowFlags;
enum {
    WM_WindowFlag_CustomBorder       = (1 << 0),
    WM_WindowFlag_UseDefaultPosition = (1 << 1)
};

typedef struct WM_Window WM_Window;
struct WM_Window {
    u64 u64[1];
};

////////////////////////////
// External Window Types

typedef struct WM_Ext_Window WM_Ext_Window;
struct WM_Ext_Window {
    u64 u64[1];
};

////////////////////
// Monitor Types

typedef struct WM_Monitor WM_Monitor;
struct WM_Monitor {
    u64 u64[1];
};

typedef struct WM_Monitor_Array WM_Monitor_Array;
struct WM_Monitor_Array {
    WM_Monitor *v;
    u64 count;
};

///////////////////
// Cursor Types

typedef u32 WM_Cursor;
enum {
    WM_Cursor_Pointer,
    WM_Cursor_IBar,
    WM_Cursor_LeftRight,
    WM_Cursor_UpDown,
    WM_Cursor_DownRight,
    WM_Cursor_UpRight,
    WM_Cursor_UpDownLeftRight,
    WM_Cursor_HandPoint,
    WM_Cursor_Disabled,
    WM_Cursor_COUNT
};

/////////////////////
// Generated Code

#include "window_manager/generated/window_manager.meta.h"

//////////////////
// Event Types

typedef u32 WM_EventKind;
enum {
    WM_EventKind_Null,
    WM_EventKind_Press,
    WM_EventKind_Release,
    WM_EventKind_MouseMove,
    WM_EventKind_Text,
    WM_EventKind_Scroll,
    WM_EventKind_WindowLoseFocus,
    WM_EventKind_WindowClose,
    WM_EventKind_FileDrop,
    WM_EventKind_Wakeup,
    WM_EventKind_COUNT
};

typedef u32 WM_Modifiers;
enum {
    WM_Modifier_Ctrl  = (1 << 0),
    WM_Modifier_Shift = (1 << 1),
    WM_Modifier_Alt   = (1 << 2)
};

typedef struct WM_Event WM_Event;
struct WM_Event {
    WM_Event *next;
    WM_Event *prev;
    u64 timestamp_us;
    WM_Window window;
    WM_EventKind kind;
    WM_Modifiers modifiers;
    WM_Key key;
    bool32 is_repeat;
    bool32 right_sided;
    u32 character;
    u32 repeat_count;
    Vec2f32 pos;
    Vec2f32 delta;
    String8_List strings;
};

typedef struct WM_Event_List WM_Event_List;
struct WM_Event_List {
    u64 count;
    WM_Event *first;
    WM_Event *last;
};

/////////////////////////////////////////////////////////
// Application Defined Frame Hook Forward Declaration

internal bool32 frame(void);

//////////////////////////
// Handle Type Helpers

internal WM_Window        wm_window_zero(void);
internal bool32           wm_window_match(WM_Window a, WM_Window b);
internal bool32           wm_monitor_match(WM_Monitor a, WM_Monitor b);

//////////////////////////////////////////////////
// Event Functions (Helpers, implemented once)

internal String8          wm_string_from_event_kind    (WM_EventKind kind);
internal String8_List     wm_string_list_from_modifiers(Arena *arena, WM_Modifiers flags);
internal String8          wm_string_from_modifiers_key (Arena *arena, WM_Modifiers modifiers, WM_Key key);
internal u32              wm_codepoint_from_modifiers_and_key(WM_Modifiers flags, WM_Key key);
internal void             wm_eat_event  (WM_Event_List *events, WM_Event *event);
internal bool32           wm_key_press  (WM_Event_List *events, WM_Window window, WM_Modifiers modifiers, WM_Key key);
internal bool32           wm_key_release(WM_Event_List *events, WM_Window window, WM_Modifiers modifiers, WM_Key key);
internal bool32           wm_text(WM_Event_List *events, WM_Window window, u32 character);
internal WM_Event_List    wm_event_list_copy(Arena *arena, WM_Event_List *src);
internal void             wm_event_list_concat_in_place(WM_Event_List *dst, WM_Event_List *to_push);
internal WM_Event        *wm_event_list_push_new(Arena *arena, WM_Event_List *evts, WM_EventKind kind);

///////////////////////////////////////////
// @per_os_impl Main Initialization API

internal void             wm_init(void);

////////////////////////////////////////
// @per_os_impl Graphics System Info

internal WM_System_Info  *wm_get_system_info(void);

//////////////////////////////
// @per_os_impl Clipboards

internal void             wm_set_clipboard_text(String8 string);
internal String8          wm_get_clipboard_text(Arena *arena);

///////////////////////////
// @per_os_impl Windows

internal WM_Window        wm_window_open(Rng2f32 rect, WM_WindowFlags flags, String8 title);
internal void             wm_window_close(WM_Window window);
internal void             wm_window_set_title(WM_Window window, String8 title);
internal void             wm_window_first_paint(WM_Window window);
internal void             wm_window_focus(WM_Window window);
internal bool32           wm_window_is_focused(WM_Window window);
internal bool32           wm_window_is_fullscreen(WM_Window window);
internal void             wm_window_set_fullscreen(WM_Window window, bool32 fullscreen);
internal bool32           wm_window_is_maximized(WM_Window window);
internal void             wm_window_set_maximized(WM_Window window, bool32 maximized);
internal bool32           wm_window_is_minimized(WM_Window window);
internal void             wm_window_set_minimized(WM_Window window, bool32 minimized);
internal void             wm_window_bring_to_front(WM_Window window);
internal void             wm_window_set_monitor(WM_Window window, WM_Monitor monitor);
internal void             wm_window_clear_custom_border_data(WM_Window handle);
internal void             wm_window_push_custom_title_bar(WM_Window handle, f32 thickness);
internal void             wm_window_push_custom_edges(WM_Window handle, f32 thickness);
internal void             wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2f32 rect);
internal Rng2f32          wm_rect_from_window(WM_Window window);
internal Rng2f32          wm_client_rect_from_window(WM_Window window);
internal f32              wm_dpi_from_window(WM_Window window);

////////////////////////////////////
// @per_os_impl External Windows

internal WM_Ext_Window    wm_focused_external_window(void);
internal void             wm_focus_external_window(WM_Ext_Window ext_window);

////////////////////////////
// @per_os_impl Monitors

internal WM_Monitor_Array wm_push_monitors_array(Arena *arena);
internal WM_Monitor       wm_primary_monitor(void);
internal WM_Monitor       wm_monitor_from_window(WM_Window window);
internal String8          wm_name_from_monitor(Arena *arena, WM_Monitor monitor);
internal Vec2f32          wm_dim_from_monitor(WM_Monitor monitor);
internal f32              wm_dpi_from_monitor(WM_Monitor monitor);

//////////////////////////
// @per_os_impl Events

internal void             wm_send_wakeup_event(void);
internal WM_Event_List    wm_get_events(Arena *arena, bool32 wait);
internal WM_Modifiers     wm_get_modifiers(void);
internal bool32           wm_key_is_down(WM_Key key);
internal Vec2f32          wm_mouse_from_window(WM_Window window);

///////////////////////////
// @per_os_impl Cursors

internal void             wm_set_cursor(WM_Cursor cursor);

/////////////////////////////////////////////////////////
// @per_os_impl Native User-Facing Graphical Messages

internal void             wm_graphical_message(bool32 error, String8 title, String8 message);
internal String8          wm_graphical_pick_file(Arena *arena, String8 title, String8 initial_path);

////////////////////////////////////
// @per_os_impl Shell Operations

internal void             wm_show_in_filesystem_ui(String8 path);
internal void             wm_open_in_browser(String8 url);

#endif // WINDOW_MANAGER_H

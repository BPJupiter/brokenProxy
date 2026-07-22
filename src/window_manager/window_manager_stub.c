

/////////////////////////////////////////////////
//~ rjf: @per_os_impl Main Initialization API

internal void wm_init(void)
{
}

//////////////////////////////////////////////
//~ rjf: @per_os_impl Graphics System Info

internal WM_System_Info *wm_get_system_info(void)
{
    local_persist WM_System_Info g = {0};
    g.default_refresh_rate = 60.f;
    return &g;
}

////////////////////////////////////
//~ rjf: @per_os_impl Clipboards

internal void wm_set_clipboard_text(String8 string)
{
}

internal String8 wm_get_clipboard_text(Arena *arena)
{
    return str8_zero();
}

/////////////////////////////////
//~ rjf: @per_os_impl Windows

internal WM_Window wm_window_open(Rng2f32 rect, WM_WindowFlags flags, String8 title)
{
    WM_Window handle = {1};
    return handle;
}

internal void wm_window_close(WM_Window window)
{
}

internal void wm_window_set_title(WM_Window window, String8 title)
{
}

internal void wm_window_first_paint(WM_Window window)
{
}

internal void wm_window_focus(WM_Window window)
{
}

internal bool32 wm_window_is_focused(WM_Window window)
{
    return false;
}

internal bool32 wm_window_is_fullscreen(WM_Window window)
{
    return false;
}

internal void wm_window_set_fullscreen(WM_Window window, bool32 fullscreen)
{
}

internal bool32 wm_window_is_maximized(WM_Window window)
{
    return false;
}

internal void wm_window_set_maximized(WM_Window window, bool32 maximized)
{
}

internal bool32 wm_window_is_minimized(WM_Window window)
{
    return false;
}

internal void wm_window_set_minimized(WM_Window window, bool32 minimized)
{
}

internal void wm_window_bring_to_front(WM_Window window)
{
}

internal void wm_window_set_monitor(WM_Window window, WM_Monitor monitor)
{
}

internal void wm_window_clear_custom_border_data(WM_Window handle)
{
}

internal void wm_window_push_custom_title_bar(WM_Window handle, f32 thickness)
{
}

internal void wm_window_push_custom_edges(WM_Window handle, f32 thickness)
{
}

internal void wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2f32 rect)
{
}

internal Rng2f32 wm_rect_from_window(WM_Window window)
{
    Rng2f32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
    return rect;
}

internal Rng2f32 wm_client_rect_from_window(WM_Window window)
{
    Rng2f32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
    return rect;
}

internal f32 wm_dpi_from_window(WM_Window window)
{
    return 96.f;
}

//////////////////////////////////////////
//~ rjf: @per_os_impl External Windows

internal WM_Ext_Window wm_focused_external_window(void)
{
    WM_Ext_Window result = {0};
    return result;
}

internal void wm_focus_external_window(WM_Ext_Window handle)
{
}

//////////////////////////////////
//~ rjf: @per_os_impl Monitors

internal WM_Monitor_Array wm_push_monitors_array(Arena *arena)
{
    WM_Monitor_Array arr = {0};
    return arr;
}

internal WM_Monitor wm_primary_monitor(void)
{
    WM_Monitor handle = {1};
    return handle;
}

internal WM_Monitor wm_monitor_from_window(WM_Window window)
{
    WM_Monitor handle = {1};
    return handle;
}

internal String8 wm_name_from_monitor(Arena *arena, WM_Monitor monitor)
{
    return str8_zero();
}

internal Vec2f32 wm_dim_from_monitor(WM_Monitor monitor)
{
    Vec2f32 v = v2f32(1000, 1000);
    return v;
}

internal f32 wm_dpi_from_monitor(WM_Monitor monitor)
{
    return 96.f;
}

////////////////////////////////
//~ rjf: @per_os_impl Events

internal void wm_send_wakeup_event(void)
{
}

internal WM_Event_List wm_get_events(Arena *arena, bool32 wait)
{
    WM_Event_List evts = {0};
    return evts;
}

internal WM_Modifiers wm_get_modifiers(void)
{
    WM_Modifiers f = 0;
    return f;
}

internal bool32 wm_key_is_down(WM_Key key)
{
    return 0;
}

internal Vec2f32 wm_mouse_from_window(WM_Window window)
{
    return v2f32(0, 0);
}

/////////////////////////////////
//~ rjf: @per_os_impl Cursors

internal void wm_set_cursor(WM_Cursor cursor)
{
}

///////////////////////////////////////////////////////////////
//~ rjf: @per_os_impl Native User-Facing Graphical Messages

internal void wm_graphical_message(bool32 error, String8 title, String8 message)
{
}

internal String8 wm_graphical_pick_file(Arena *arena, String8 title, String8 initial_path)
{
    return str8_zero();
}

//////////////////////////////////////////
//~ rjf: @per_os_impl Shell Operations

internal void wm_show_in_filesystem_ui(String8 path)
{
}

internal void wm_open_in_browser(String8 url)
{
}

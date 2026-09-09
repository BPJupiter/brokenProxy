
/////////////////////
// Generated Code

#include "generated/tide.meta.c"

/////////////////////////////
// Registers Type Functions


internal void ti_regs_copy_contents(Arena *arena, TI_Regs *dst, TI_Regs *src)
{
    MemoryCopyStruct(dst, src);
    /*
    dst->cfg_list    = cfg_id_list_copy(arena, &src->cfg_list);
    dst->file_path   = push_str8_copy(arena, src->file_path);
    dst->lines       = d_line_list_copy(arena, &src->lines);
    dst->expr        = push_str8_copy(arena, src->expr);
    dst->string      = push_str8_copy(arena, src->string);
    dst->cmd_name    = push_str8_copy(arena, src->cmd_name);
    if(dst->cfg_list.count == 0 && dst->cfg != 0)
    {
        cfg_id_list_push(arena, &dst->cfg_list, dst->cfg);
    }
    */
}

internal TI_Regs *
ti_regs_copy(Arena *arena, TI_Regs *src)
{
    TI_Regs *dst = push_array(arena, TI_Regs, 1);
    ti_regs_copy_contents(arena, dst, src);
    return dst;
}

//////////////////////
// Commands Type Functions

internal void ti_cmd_list_push_new(Arena *arena, TI_Cmd_List *cmds, String8 name, TI_Regs *regs)
{
    TI_Cmd_Node *n = push_array(arena, TI_Cmd_Node, 1);
    n->cmd.name = push_str8_copy(arena, name);
    n->cmd.regs = ti_regs_copy(arena, regs);
    DLLPushBack(cmds->first, cmds->last, n);
    cmds->count += 1;
}

//////////////////////
// Config Functions

/*
internal bool32 ti_cfg_is_project_filtered(CFG_Node *cfg)
{
    CFG_Node *project = cfg_node_child_from_string(cfg, str8_lit("project"));
    bool32 result = (project != &cfg_nil_node &&
                     project->first->string.size != 0 &&
                     !patch_match_normalised(ti_state->project_path, project->first->string));
    return result;
}
*/

internal Vec4f32 ti_hsva_from_cfg(CFG_Node *cfg)
{
    Vec4f32 hsva = {0};
    CFG_Node *hsva_root = cfg_node_child_from_string(cfg, str8_lit("hsva"));
    CFG_Node *h = hsva_root->first;
    CFG_Node *s = h->next;
    CFG_Node *v = s->next;
    CFG_Node *a = v->next;
    hsva.x = (f32)f64_from_str8(h->string);
    hsva.y = (f32)f64_from_str8(s->string);
    hsva.z = (f32)f64_from_str8(v->string);
    hsva.w = (f32)f64_from_str8(a->string);
    return hsva;
}

internal Vec4f32
ti_color_from_cfg(CFG_Node *cfg)
{
  Vec4f32 hsva = ti_hsva_from_cfg(cfg);
  Vec4f32 rgba = linear_from_srgba(rgba_from_hsva(hsva));
  return rgba;
}

/*
internal bool32
ti_disabled_from_cfg(CFG_Node *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *child_schema = &md_nil_node;
  MD_Node_Ptr_List schemas = cfg_schemas_from_name(scratch.arena, ti_state->cfg_schema_table, cfg->string);
  for(MD_Node_Ptr_Node *n = schemas.first; n != 0 && child_schema == &md_nil_node; n = n->next)
  {
    child_schema = md_child_from_string(n->v, str8_lit("enabled"), 0);
  }
  MD_Node *default_tag = md_tag_from_string(child_schema, str8_lit("default"), 0);
  String8 value_string = cfg_node_child_from_string(cfg, str8_lit("enabled"))->first->string;
  if(value_string.size == 0)
  {
    value_string = default_tag->first->string;
  }
  bool32 is_enabled = !!e_value_from_string(value_string).u64;
  bool32 is_disabled = !is_enabled;
  if(value_string.size == 0)
  {
    is_disabled = 0;
  }
  scratch_end(scratch);
  return is_disabled;
}
*/

internal String8
ti_name_from_cfg(CFG_Node *cfg)
{
  CFG_Node *name_root = cfg_node_child_from_string(cfg, str8_lit("name"));
  String8 result = name_root->first->string;
  return result;
}

internal String8
ti_label_from_cfg(CFG_Node *cfg)
{
  CFG_Node *label_root = cfg_node_child_from_string(cfg, str8_lit("label"));
  String8 result = label_root->first->string;
  return result;
}

internal String8
ti_path_from_cfg(CFG_Node *cfg)
{
  CFG_Node *root = cfg_node_child_from_string(cfg, str8_lit("path"));
  String8 result = root->first->string;
  return result;
}

/*
internal String8 ti_default_setting_from_names(String8 schema_name, String8 setting_name);

internal String8
ti_setting_from_name(String8 name)
{
  String8 result = {0};
  if(name.size != 0)
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: find most-granular config scopes to begin looking for the setting
    typedef struct Cfg_Seed_Task Cfg_Seed_Task;
    struct Cfg_Seed_Task
    {
      Cfg_Seed_Task *next;
      CFG_Node *cfg;
      bool32 allow_bucket_chains;
    };
    CFG_Node *view_cfg = cfg_node_from_id(ti_regs()->view);
    if(view_cfg == &cfg_nil_node)
    {
      view_cfg = cfg_node_from_id(ti_regs()->tab);
    }
    Cfg_Seed_Task panel_task = {0, &cfg_nil_node, 1};
    if(panel_task.cfg == &cfg_nil_node) { panel_task.cfg = cfg_node_from_id(ti_regs()->panel); }
    if(panel_task.cfg == &cfg_nil_node) { panel_task.cfg = cfg_node_from_id(ti_regs()->window); }
    if(panel_task.cfg == &cfg_nil_node) { panel_task.cfg = cfg_node_child_from_string(cfg_node_root(), s("user")); }
    Cfg_Seed_Task view_task = {&panel_task, view_cfg, 1};
    Cfg_Seed_Task *first_task = &view_task;
    Cfg_Seed_Task *last_task = &panel_task;
    
    // rjf: for each task, look for the setting, follow parent chain upwatis
    CFG_Node *setting = &cfg_nil_node;
    for(Cfg_Seed_Task *t = first_task; t != 0; t = t->next)
    {
      for(CFG_Node *cfg = t->cfg; cfg != &cfg_nil_node; cfg = cfg->parent)
      {
        setting = cfg_node_child_from_string(cfg, name);
        if(setting != &cfg_nil_node)
        {
          goto break_all;
        }
        if(cfg->parent == cfg_node_root() && t->allow_bucket_chains)
        {
          String8 next_bucket = {0};
          bool32 allow_bucket_chains = 0;
          if(str8_match(cfg->string, str8_lit("user"), 0))
          {
            next_bucket = str8_lit("project");
          }
          else if(str8_match(cfg->string, str8_lit("project"), 0))
          {
            next_bucket = str8_lit("user");
          }
          else
          {
            allow_bucket_chains = 1;
            next_bucket = str8_lit("user");
          }
          if(next_bucket.size != 0)
          {
            Cfg_Seed_Task *task = push_array(scratch.arena, Cfg_Seed_Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->cfg = cfg_node_child_from_string(cfg_node_root(), next_bucket);
            task->allow_bucket_chains = allow_bucket_chains;
          }
        }
      }
    }
    break_all:;
    
    // rjf: return resultant child string stored under this key
    result = setting->first->string;
    
    // rjf: no result -> look for default in schemas
    if(result.size == 0)
    {
      for(Cfg_Seed_Task *t = first_task; t != 0; t = t->next)
      {
        for(CFG_Node *cfg = t->cfg; cfg != &cfg_nil_node; cfg = cfg->parent)
        {
          result = ti_default_setting_from_names(cfg->string, name);
          if(result.size != 0)
          {
            goto break_all2;
          }
        }
      }
      break_all2:;
    }
    
    scratch_end(scratch);
  }
  return result;
}


internal bool32 ti_setting_bool32_from_name(String8 name)
{
    bool32 result = false;
    String8 value = ti_setting_from_name(name);
    if (value.size != 0)
    {
        u64 value_u64 = 0;
        try_u64_from_str8_c_rules(value, &value_u64);
        result = !!value_u64;
    }
    return result;
}

internal u64 ti_setting_u64_from_name(String8 name)
{
    u64 result = 0;
    String8 value = ti_setting_from_name(name);
    if (value.size != 0)
    {
        try_u64_from_str8_c_rules(value, &result);
    }
    return result;
}

internal f32 ti_setting_f32_from_name(String8 name)
{
    f32 result = 0.f;
    String8 value = ti_setting_from_name(name);
    if (value.size != 0)
    {
        if (value.str[value.size - 1] == 'f' || value.str[value.size - 1] == 'F')
        {
            value = str8_chop(value, 1);
        }
        result = (f32)f64_from_str8(value);
    }
    return result;
}

*/

internal CFG_Node *ti_immediate_cfg_from_key(String8 string)
{
    CFG_Node *transient = cfg_node_child_from_string(cfg_node_root(), str8_lit("transient"));
    CFG_Node *immediate = &cfg_nil_node;
    CFG_Node *cfg = &cfg_nil_node;
    for (CFG_Node *child = transient->first; child != &cfg_nil_node; child = child->next)
    {
        if (str8_match(child->string, str8_lit("immediate"), 0))
        {
            cfg = cfg_node_child_from_string(child, string);
            if (cfg != &cfg_nil_node)
            {
                immediate = child;
                break;
            }
        }
    }
    if (cfg == &cfg_nil_node)
    {
        immediate = cfg_node_new(ti_state->cfg, transient, str8_lit("immediate"));
        cfg = cfg_node_new(ti_state->cfg, immediate, string);
    }
    cfg_node_child_from_string_or_alloc(ti_state->cfg, immediate, str8_lit("hot"));
    return cfg;
}

internal CFG_Node *ti_immediate_cfg_from_keyf(char *fmt, ...)
{
    Temp scratch = scratch_begin(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 key = push_str8fv(scratch.arena, fmt, args);
    CFG_Node *result = ti_immediate_cfg_from_key(key);
    va_end(args);
    scratch_end(scratch);
    return result;
}

///////////////////////
// Window Functions

internal String8 ti_push_window_title(Arena *arena)
{
    CFG_Node *root = cfg_node_root();
    CFG_Node *project = cfg_node_child_from_string(root, str8_lit("project"));
    CFG_Node *name = cfg_node_child_from_string(project, str8_lit("name"));
    String8 project_name = name->first->string;
    if (project_name.size == 0)
    {
        /*
        String8 prof_path = ti_state->project_path;
        prof_path = str8_chop_last_dot(prof_path);
        project_name = str8_skip_last_slash(prof_path);
        */
    }
    String8 result = push_str8f(arena,
                                "%S%s%s",
                                project_name,
                                project_name.size != 0 ? " - " : "",
                                BUILD_TITLE " (" BUILD_VERSION_STRING_LITERAL " " BUILD_RELEASE_PHASE_STRING_LITERAL ")");
    return result;
}

internal CFG_Node *ti_window_from_cfg(CFG_Node *cfg)
{
    CFG_Node *result = &cfg_nil_node;
    for (CFG_Node *c = cfg; c != &cfg_nil_node; c = c->parent)
    {
        if (c->parent->parent == cfg_node_root() && str8_match(c->string, str8_lit("window"), 0))
        {
            result = c;
            break;
        }
    }
    return result;
}

internal TI_Window_State *ti_window_state_from_cfg(CFG_Node *cfg)
{
    // unpack
    CFG_Node *window_cfg = ti_window_from_cfg(cfg);
    CFG_ID id = window_cfg->id;

    // scan for existing window
    TI_Window_State *ws = &ti_nil_window_state;
    if (id != 0 &&
        id == ti_state->window_state_last_accessed_id &&
        id == ti_state->window_state_last_accessed->cfg_id)
    {
        ws = ti_state->window_state_last_accessed;
    }
    else
    {
        u64 hash = u64_hash_from_str8(str8_struct(&id));
        u64 slot_idx = hash % ti_state->window_state_slots_count;
        TI_Window_State_Slot *slot = &ti_state->window_state_slots[slot_idx];
        for (TI_Window_State *w = slot->first; w != 0; w = w->hash_next)
        {
            if (w->cfg_id == id)
            {
                ws = w;
                break;
            }
        }
    }

    // allocate/open new window if one was not found
    if (window_cfg != &cfg_nil_node && ws == &ti_nil_window_state)
    {
        Temp scratch = scratch_begin(0, 0);

        // unpack configuration options
        bool32 has_pos = 0;
        Vec2f32 pos = {0};
        Vec2f32 size = {0};
        WM_Monitor preferred_monitor = {0};
        {
            CFG_Node *pos_cfg = cfg_node_child_from_string(window_cfg, str8_lit("pos"));
            has_pos = (pos_cfg != &cfg_nil_node);
            CFG_Node *size_cfg = cfg_node_child_from_string(window_cfg, str8_lit("size"));
            CFG_Node *monitor_cfg = cfg_node_child_from_string(window_cfg, str8_lit("monitor"));
            pos.x = (f32)f64_from_str8(pos_cfg->first->string);
            pos.y = (f32)f64_from_str8(pos_cfg->first->next->string);
            size.x = (f32)f64_from_str8(size_cfg->first->string);
            size.y = (f32)f64_from_str8(size_cfg->first->next->string);
            WM_Monitor_Array monitors = wm_push_monitors_array(scratch.arena);
            for EachIndex(idx, monitors.count)
            {
                String8 monitor_name = wm_name_from_monitor(scratch.arena, monitors.v[idx]);
                if (str8_match(monitor_name, monitor_cfg->first->string, StringMatchFlag_CaseInsensitive))
                {
                    preferred_monitor = monitors.v[idx];
                    break;
                }
            }
        }

        // allocate window
        ws = ti_state->free_window_state;
        if (ws != 0)
        {
            SLLStackPop_N(ti_state->free_window_state, order_next);
        }
        else
        {
            ws = push_array_no_zero(ti_state->arena, TI_Window_State, 1);
        }
        MemoryZeroStruct(ws);

        // fill out window
        ws->cfg_id = id;
        ws->arena = arena_alloc();
        {
            String8 title = ti_push_window_title(scratch.arena);
            ws->os = wm_window_open(r2f32p(pos.x, pos.y, pos.x + size.x, pos.y + size.y),
                                    // WM_WindowFlag_CustomBorder
                                    (!has_pos * WM_WindowFlag_UseDefaultPosition), title);
        }
        ws->r = r_window_equip(ws->os);
        ws->ui = ui_state_alloc();
        WM_Monitor zero_monitor = {0};
        if (!wm_monitor_match(zero_monitor, preferred_monitor))
        {
            wm_window_set_monitor(ws->os, preferred_monitor);
        }
        if (cfg_node_child_from_string(window_cfg, str8_lit("fullscreen")) != &cfg_nil_node)
        {
            wm_window_set_fullscreen(ws->os, 1);
        }
        if (cfg_node_child_from_string(window_cfg, str8_lit("maximized")) != &cfg_nil_node)
        {
            wm_window_set_maximized(ws->os, 1);
        }

        // hook up window links
        u64 hash = u64_hash_from_str8(str8_struct(&id));
        u64 slot_idx = hash % ti_state->window_state_slots_count;
        TI_Window_State_Slot *slot = &ti_state->window_state_slots[slot_idx];
        DLLPushBack_NPZ(&ti_nil_window_state, ti_state->first_window_state, ti_state->last_window_state, ws, order_next, order_prev);
        DLLPushBack_NP(slot->first, slot->last, ws, hash_next, hash_prev);

        scratch_end(scratch);
    }

    // touch window for this frame
    if (ws != &ti_nil_window_state)
    {
        ws->last_frame_index_touched = ti_state->frame_index;
    }

    ti_state->window_state_last_accessed_id = ws->cfg_id;
    ti_state->window_state_last_accessed = ws;
    return ws;
}

internal TI_Window_State *ti_window_state_from_os_handle(WM_Window os)
{
    TI_Window_State *ws = &ti_nil_window_state;
    {
        for (TI_Window_State *w = ti_state->first_window_state;
             w != &ti_nil_window_state;
             w = w->order_next)
        {
            if (wm_window_match(w->os, os))
            {
                ws = w;
                break;
            }
        }
    }
    return ws;
}

#if COMPILER_MSVC && !BUILD_DEBUG
NO_OPTIMIZE_BEGIN
#endif

internal void ti_window_frame(void)
{
    Temp scratch = scratch_begin(0, 0);
    ProfBeginFunction();

    /////////////////////////////////////
    // @window_frame_part unpack context
    //
    CFG_Node *window          = cfg_node_from_id(ti_regs()->window);
    TI_Window_State *ws       = ti_window_state_from_cfg(cfg_node_from_id(ti_regs()->window));
    CFG_Panel_Tree panel_tree = cfg_panel_tree_from_cfg(scratch.arena, window);
    bool32 window_is_focused  = wm_window_is_focused(ws->os);

    ui_select_state(ws->ui);

    /////////////////////////////
    // @window_frame_part fill panel/view interaction registers
    //
    {}

    /////////////////////////////////////
    // @window_frame_part compute window's frame
    //
    {
        Access *access = access_open();

        typedef struct TI_Color_Def TI_Color_Def;
        struct TI_Color_Def
        {
            char *tags[2];
            u32   tag_count;
            u32   srgba;
        };
        TI_Color_Def defs[] = {
            { {"background"},           1, 0x14161AFF }, // window clear color - near-black
            { {"border"},                1, 0x4A4E58FF }, // window border / box borders - mid gray
            { {"drop_shadow"},           1, 0x000000AA }, // shadow under floating/hot boxes
            { {"text"},                  1, 0xF2F2F2FF }, // default text - near-white
            { {"text", "weak"},          2, 0x9AA0AAFF }, // de-emphasized text/icons
            { {"hover"},                 1, 0x3C82F6FF }, // hot/hover highlight - blue
            { {"focus", "overlay"},      2, 0x3C82F633 }, // focus fill overlay - translucent blue
            { {"focus", "border"},       2, 0x3C82F6FF }, // focus outline - solid blue
            { {"bad", "text"},           2, 0xFF5C5CFF }, // error state text/border - red
            { {"match", "background"},   2, 0xF6C64555 }, // fuzzy-match highlight - amber
        };
        
        // convert to final pattern array
        ws->theme = push_array(ti_frame_arena(), UI_Theme, 1);
        ws->theme->patterns = push_array(ti_frame_arena(), UI_Theme_Pattern, ArrayCount(defs));
        ws->theme->patterns_count = ArrayCount(defs);
        for (u64 i = 0; i < ArrayCount(defs); i += 1)
        {
            TI_Color_Def *def = &defs[i];
            String8 *tag_strs = push_array(ti_frame_arena(), String8, def->tag_count);
            for (u32 t = 0; t < def->tag_count; t += 1)
            {
                tag_strs[t] = str8_cstring(def->tags[t]);
            }
            ws->theme->patterns[i].tags  = (String8_Array){tag_strs, def->tag_count};
            ws->theme->patterns[i].linear = linear_from_srgba(rgba_from_u32(def->srgba));
        }

        access_close(access);
    }

    /////////////////////////
    // @window_frame_part commit window's position/status to underlying cfg tree
    //
    {
        Temp scratch = scratch_begin(0, 0);
        bool32 is_fullscreen = wm_window_is_fullscreen(ws->os);
        bool32 is_maximized = wm_window_is_maximized(ws->os);
        bool32 is_minimized = wm_window_is_minimized(ws->os);
        if (is_fullscreen)
        {
            cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("fullscreen"));
        }
        else
        {
            cfg_node_release(ti_state->cfg, cfg_node_child_from_string(window, str8_lit("fullscreen")));
        }
        if (is_maximized)
        {
            cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("maximized"));
        }
        else
        {
            cfg_node_release(ti_state->cfg, cfg_node_child_from_string(window, str8_lit("maximized")));
        }

        // dpi changes -> xform font size / window size
        f32 dpi = wm_dpi_from_window(ws->os);
        if (dpi != ws->last_dpi)
        {
            fnt_reset();
            f32 current_font_size = ti_font_size();
            f32 new_font_size = current_font_size * (dpi / ws->last_dpi);
            new_font_size = Clamp(6.f, new_font_size, 72.f);
            CFG_Node *font_size_cfg = cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("font_size"));
            cfg_node_new_replacef(ti_state->cfg, font_size_cfg, "%I64u", (u64)new_font_size);
            ws->last_dpi = dpi;
        }

        // commit position
        Rng2f32 window_rect = wm_rect_from_window(ws->os);
        if (!is_fullscreen && !is_maximized && !is_minimized)
        {
            Vec2f32 pos = window_rect.p0;
            CFG_Node *pos_root = cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("pos"));
            if ((s32)pos.x != (s32)f64_from_str8(pos_root->first->string) ||
                (s32)pos.y != (s32)f64_from_str8(pos_root->last->string))
            {
                CFG_Node *x = pos_root->first;
                if (x == &cfg_nil_node)
                {
                    x = cfg_node_alloc(ti_state->cfg);
                    cfg_node_insert_child(ti_state->cfg, pos_root, &cfg_nil_node, x);
                }
                CFG_Node *y = x->next;
                if (y == &cfg_nil_node)
                {
                    y = cfg_node_alloc(ti_state->cfg);
                    cfg_node_insert_child(ti_state->cfg, pos_root, x, y);
                }
                cfg_node_equip_stringf(ti_state->cfg, x, "%i", (s32)pos.x);
                cfg_node_equip_stringf(ti_state->cfg, y, "%i", (s32)pos.y);
            }
        }

        // commit size
        if (!is_fullscreen && !is_maximized && !is_minimized)
        {
            Vec2f32 size = dim_2f32(window_rect);
            CFG_Node *size_root = cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("size"));
            if((s32)size.x != (s32)f64_from_str8(size_root->first->string) ||
               (s32)size.y != (s32)f64_from_str8(size_root->last->string))
            {
                CFG_Node *width = size_root->first;
                if(width == &cfg_nil_node)
                {
                    width = cfg_node_alloc(ti_state->cfg);
                    cfg_node_insert_child(ti_state->cfg, size_root, &cfg_nil_node, width);
                }
                CFG_Node *height = width->next;
                if(height == &cfg_nil_node)
                {
                    height = cfg_node_alloc(ti_state->cfg);
                    cfg_node_insert_child(ti_state->cfg, size_root, width, height);
                }
                cfg_node_equip_stringf(ti_state->cfg, width, "%i", (s32)size.x);
                cfg_node_equip_stringf(ti_state->cfg, height, "%i", (s32)size.y);
            }
        }
        
        
        // commit monitor
        if (!is_minimized)
        {
            WM_Monitor monitor = wm_monitor_from_window(ws->os);
            String8 monitor_name = wm_name_from_monitor(scratch.arena, monitor);
            CFG_Node *monitor_root = cfg_node_child_from_string_or_alloc(ti_state->cfg, window, str8_lit("monitor"));
            if (!str8_match(monitor_root->first->string, monitor_name, 0))
            {
                cfg_node_new_replace(ti_state->cfg, monitor_root, monitor_name);
            }
        }
        scratch_end(scratch);
    }
    
    //////////////////////////////
    // @window_frame_part build UI
    ProfScope("build UI")
    {
        //////////////////////////
        // @window_ui_part set up
        {
            // get font size info
            f32 font_size = 16.f;

            // build icon info
            UI_Icon_Info icon_info = {0};
            {
            }

            // build animation info
            UI_Animation_Info animation_info = {0};
            {
                animation_info.hot_animation_rate      = ti_state->catchall_animation_rate;
                animation_info.active_animation_rate   = ti_state->catchall_animation_rate;
                animation_info.focus_animation_rate    = 1.f;
                animation_info.tooltip_animation_rate  = ti_state->tooltip_animation_rate;
                animation_info.menu_animation_rate     = ti_state->menu_animation_rate;
                animation_info.scroll_animation_rate   = ti_state->scrolling_animation_rate;
            }

            // begin & push initial stack values
            ui_begin_build(ws->os,
                           &ws->ui_events,
                           &icon_info,
                           ws->theme,
                           &animation_info,
                           ti_state->frame_dt,
                           ti_state->frame_dt);
            ui_push_font(fnt_tag_from_static_data_string(&ti_default_main_font_bytes));
            ui_push_font_size(font_size);
            ui_push_text_padding(floor_f32(ui_top_font_size() * 0.3f));
            ui_push_pref_width(ui_px(floor_f32(ui_top_font_size() * 20.f), 1.f));
            ui_push_pref_height(ui_px(floor_f32(ui_top_font_size() * 3.f), 1.f));
            ui_push_blur_size(10.f);
        }

        // @window_ui_part calculate top-level rectangles/sizes
        f32 window_edge_px = ui_top_font_size() * 0.2f;
        Rng2f32 window_rect = wm_client_rect_from_window(ws->os);
        Vec2f32 window_rect_dim = dim_2f32(window_rect);
        f32 top_bar_dim_px = floor_f32(ui_top_font_size() * 3.f);
        Rng2f32 top_bar_rect = r2f32p(window_rect.x0,
                                      window_rect.y0,
                                      window_rect.x0 + window_rect_dim.x,
                                      window_rect.y0 + top_bar_dim_px);
        Rng2f32 bottom_bar_rect = r2f32p(window_rect.x0,
                                         window_rect_dim.y - top_bar_dim_px,
                                         window_rect.x0 + window_rect_dim.x,
                                         window_rect.y0 + window_rect_dim.y);
        Rng2f32 content_rect = r2f32p(window_rect.x0,
                                      top_bar_rect.y1 + window_edge_px * 1.5f,
                                      window_rect.x0 + window_rect_dim.x,
                                      bottom_bar_rect.y0);
        content_rect = pad_2f32(content_rect, -window_edge_px);

        

        //////////////////////////////////////////
        // @window_ui_part button test
        if (DEV_button_test)
        {
            UI_TextAlignment(UI_TextAlign_Center) UI_Focus(UI_FocusKind_Root)
            {
                Vec2f32 content_dim = dim_2f32(content_rect);
                UI_Box *bg_box = &ui_nil_box;
                Vec4f32 shadow_color = ui_color_from_name(str8_lit("drop_shadow"));
                shadow_color.w += (1.f - shadow_color.w) * 0.5f;
                UI_Rect(content_rect)
                    UI_ChildLayoutAxis(Axis2_X)
                    UI_Focus(UI_FocusKind_On)
                    UI_TagF("floating")
                    UI_BackgroundColor(shadow_color)
                {
                    bg_box = ui_build_box_from_stringf(UI_BoxFlag_FixedSize|
                                                       UI_BoxFlag_Floating|
                                                       UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_Scroll|
                                                       UI_BoxFlag_DefaultFocusNav|
                                                       UI_BoxFlag_DisableFocusOverlay|
                                                       UI_BoxFlag_DisableFocusBorder|
                                                       UI_BoxFlag_DrawBackground, "###popup");
                }
                UI_Parent(bg_box)
                {
                    UI_WidthFill UI_PrefHeight(ui_children_sum(1.f)) UI_Column UI_Padding(ui_pct(1, 0)) UI_TagF("floating")
                    {
                        ui_set_next_pref_width(ui_children_sum(1));
                        ui_set_next_pref_height(ui_children_sum(1));
                        ui_set_next_child_layout_axis(Axis2_Y);
                        UI_Box *panel = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow, "");
                        UI_Parent(panel)
                        {
                            ui_spacer(ui_em(1.5f, 1.f));
                            UI_FontSize(ui_top_font_size()) UI_PrefHeight(ui_em(3.f, 1.f)) ui_label(s("I'm the label!"));
                            UI_PrefHeight(ui_em(3.f, 1.f)) UI_TagF("weak") ui_label(s("I'm the popup description!"));
                            ui_spacer(ui_em(1.5f, 1.f));
                            UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_PrefWidth(ui_em(16.f, 1.f)) UI_PrefHeight(ui_em(3.5f, 1.f)) UI_CornerRadius(ui_top_font_size() * 0.5f)
                            {
                                UI_TagF("pop")
                                if (ui_clicked(ui_buttonf("Open a new window")) || ui_slot_press(UI_EventActionSlot_Accept))
                                {
                                    ti_cmd(TI_CmdKind_OpenWindow);
                                }
                                ui_spacer(ui_em(1.f, 1.f));
                                if (ui_clicked(ui_buttonf("Close this window")) || ui_slot_press(UI_EventActionSlot_Cancel))
                                {
                                    ti_cmd(TI_CmdKind_CloseWindow);
                                }
                            }
                            ui_spacer(ui_em(3.f, 1.f));
                        }
                    }
                }
                ui_signal_from_box(bg_box);
            }
        }

        if (DEV_draw_3D_test)
        {
            ti_test_view_ui_cube(window_rect);
        }

        ///////////////////
        // @window_ui_part top bar
        //
        {}

        ///////////////////////
        // @window_ui_part bottom bar
        //
        {}
        
            
        ui_end_build();
    }

    //////////////////////
    // @window_frame_part animate
    //
    if (ui_animating_from_state(ws->ui))
    {
        ti_request_frame();
    }

    ///////////////////////////////
    // @window_frame_part draw UI
    //
    ws->draw_bucket = dr_bucket_make();
    DR_BucketScope(ws->draw_bucket)
        ProfScope("draw UI")
    {
        Temp scratch = scratch_begin(0, 0);
        f32 box_squish_epsilon = 0.001f;
        Rng2f32 window_rect = wm_client_rect_from_window(ws->os);

        // unpack settings
        f32 rounded_corner_amount = 0.5f;
        f32 border_softness       = 1.f;
        bool32 do_background_blur                = true;
        bool32 force_opaque_floating_backgrounds = true;
        bool32 do_drop_shadows                   = true;
        Vec4f32 base_background_color = ui_color_from_name(str8_lit("background"));
        Vec4f32 base_border_color     = ui_color_from_name(str8_lit("border"));
        Vec4f32 drop_shadow_color     = ui_color_from_name(str8_lit("drop_shadow"));

        // set up heatmap buckets
        f32 heatmap_bucket_size = 32.f;
        u64 *heatmap_buckets = 0;
        u64 heatmap_bucket_pitch = 0;
        u64 heatmap_bucket_count = 0;
        if (DEV_draw_ui_box_heatmap)
        {
            Rng2f32 rect = wm_client_rect_from_window(ws->os);
            Vec2f32 size = dim_2f32(rect);
            Vec2s32 buckets_dim = {(s32)(size.x/heatmap_bucket_size), (s32)(size.y/heatmap_bucket_size)};
            heatmap_bucket_pitch = buckets_dim.x;
            heatmap_bucket_count = buckets_dim.x * buckets_dim.y;
            heatmap_buckets = push_array(scratch.arena, u64, heatmap_bucket_count);
        }

        // draw background color
        {
            dr_rect(wm_client_rect_from_window(ws->os), base_background_color, 0, 0, 0);
        }

        // draw window border
        {
            dr_rect(wm_client_rect_from_window(ws->os), base_border_color, 0, 1.f, border_softness * 0.5f);
        }

        if (DEV_draw_diag_line_test)
        {
            Rng2f32 rect = wm_client_rect_from_window(ws->os);

            ti_draw_line(v2f32(rect.x0 + 20, rect.y0 + 20),
                         v2f32(rect.x1 - 20, rect.y1 - 20),
                         3.f,
                         v4f32(1, 0, 0, 1));
        }

        
        

        // recurse & draw
        u64 total_heatmap_sum_count = 0;
        UI_Box *hover_debug_box = &ui_nil_box;
        for (UI_Box *box = ui_root_from_state(ws->ui); !ui_box_is_nil(box);)
        {
            // get corner radii
            f32 box_corner_radii[Corner_COUNT] = {
                box->corner_radii[Corner_00] * rounded_corner_amount,
                box->corner_radii[Corner_01] * rounded_corner_amount,
                box->corner_radii[Corner_10] * rounded_corner_amount,
                box->corner_radii[Corner_11] * rounded_corner_amount,
            };

            // get recursion
            UI_Box_Rec rec = ui_box_rec_df_post(box, &ui_nil_box);

            // sum to box heatmap
            if (DEV_draw_ui_box_heatmap)
            {
                Vec2f32 center = center_2f32(box->rect);
                Vec2s32 p = v2s32(center.x / heatmap_bucket_size, center.y / heatmap_bucket_size);
                u64 bucket_idx = p.y * heatmap_bucket_pitch + p.x;
                if (bucket_idx < heatmap_bucket_count)
                {
                    heatmap_buckets[bucket_idx] += 1;
                    total_heatmap_sum_count += 1;
                }
            }

            // grab if debug
            if (box->flags & UI_BoxFlag_Debug && contains_2f32(box->rect, ui_mouse()))
            {
                hover_debug_box = box;
            }
            
            // push transparency
            if (box->transparency != 0)
            {
                dr_push_transparency(box->transparency);
            }

            // push squish
            if (box->squish > box_squish_epsilon)
            {
                Vec2f32 box_dim = dim_2f32(box->rect);
                Vec2f32 anchor_off = {0};
                if (box->flags & UI_BoxFlag_SquishAnchored)
                {
                    anchor_off.x = box_dim.x/2.f;
                }
                else
                {
                    anchor_off.y = -box_dim.y/8.f;
                }
                Mat3x3f32 box2origin_xform = make_translate_3x3f32(v2f32(-box->rect.x0 - box_dim.x/2 + anchor_off.x,
                                                                         -box->rect.y0               + anchor_off.y));
                Mat3x3f32 scale_xform = make_scale_3x3f32(v2f32(1 - box->squish, 1 - box->squish));
                Mat3x3f32 origin2box_xform = make_translate_3x3f32(v2f32(box->rect.x0 + box_dim.x/2 - anchor_off.x,
                                                                         box->rect.y0               - anchor_off.y));
                Mat3x3f32 xform = mul_3x3f32(origin2box_xform, mul_3x3f32(scale_xform, box2origin_xform));
                dr_push_xform2d(xform);
                dr_push_tex2d_sample_kind(R_Tex2DSampleKind_Linear);
            }

            // draw drop shadow
            if (do_drop_shadows && box->flags & UI_BoxFlag_DrawDropShadow)
            {
                Rng2f32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
                R_Rect2D_Inst *inst = dr_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
                MemoryCopyArray(inst->corner_radii, box_corner_radii);
            }

            // blur background
            if (do_background_blur && box->flags & UI_BoxFlag_DrawBackgroundBlur)
            {
                R_Pass_Params_Blur *params = dr_blur(pad_2f32(box->rect, 1.f),
                                                     box->blur_size * (1 - box->transparency),
                                                     0);
                MemoryCopyArray(params->corner_radii, box_corner_radii);
            }

            // compute effective active t
            f32 effective_active_t = box->active_t;
            if (!(box->flags & UI_BoxFlag_DrawActiveEffects))
            {
                effective_active_t = 0;
            }
            f32 t = box->hot_t * (1 - effective_active_t);

            // compute background color
            Vec4f32 box_background_color = box->background_color;
            if (force_opaque_floating_backgrounds &&
                box->flags & UI_BoxFlag_Floating && box->flags & UI_BoxFlag_DrawDropShadow)
            {
                box_background_color.w = 1.f;
            }

            // draw background
            if (box->flags & UI_BoxFlag_DrawBackground)
            {
                Rng2f32 box_bg_rect = r2f32p(box->rect.x0 + box->visual_margin.x,
                                             box->rect.y0 + box->visual_margin.y,
                                             box->rect.x1 - box->visual_margin.x,
                                             box->rect.y1 - box->visual_margin.y);

                // hot effect extension (drop shadow)
                if (box->flags & UI_BoxFlag_DrawHotEffects)
                {
                    Rng2f32 drop_shadow_rect = shift_2f32(pad_2f32(box_bg_rect, 8), v2f32(4, 4));
                    Vec4f32 color = drop_shadow_color;
                    color.w *=t * box_background_color.w;
                    dr_rect(drop_shadow_rect, color, 0.8f, 0, 8.f);
                }

                // draw background
                R_Rect2D_Inst *inst = dr_rect(pad_2f32(box_bg_rect, 1.f),
                                              box_background_color, 0, 0, border_softness * 1.f);
                MemoryCopyArray(inst->corner_radii, box_corner_radii);

                // hot effect extension
                if (box->flags & UI_BoxFlag_DrawHotEffects)
                {
                    bool32 is_hot = !ui_key_match(box->key, ui_key_zero()) && ui_key_match(box->key, ui_hot_key());
                    Vec4f32 hover_color = ui_color_from_tags_key_name(box->tags_key, str8_lit("hover"));

                    // brighten
                    if (is_hot)
                    {
                        Vec4f32 color = hover_color;
                        color.w *= 0.015f;
                        R_Rect2D_Inst *inst = dr_rect(pad_2f32(box_bg_rect, 1.f),
                                                      v4f32(0, 0, 0, 0), 0, 0, border_softness * 1.f);
                        inst->colors[Corner_00] = color;
                        inst->colors[Corner_10] = color;
                        inst->colors[Corner_01] = color;
                        inst->colors[Corner_11] = color;
                        MemoryCopyArray(inst->corner_radii, box_corner_radii);
                    }

                    // soft circle around mouse
                    if (box->hot_t > 0.01f && dim_2f32(box->rect).x > box->font_size * 8.f) DR_ClipScope(intersect_2f32(box_bg_rect, dr_top_clip()))
                    {
                        Vec4f32 color = hover_color;
                        color.w *= 0.025f;
                        if (!is_hot)
                        {
                            color.w *= t;
                        }
                        Vec2f32 center = ui_mouse();
                        Vec2f32 box_dim = dim_2f32(box->rect);
                        f32 max_dim = Max(box_dim.x, box_dim.y);
                        f32 radius = box->font_size * 24.f;
                        radius = Min(max_dim, radius);
                        dr_rect(pad_2f32(r2f32(center, center), radius * 2.f), color, radius, 0, radius / 2.f);
                    }
                }

                // active effect extension
                if (box->flags & UI_BoxFlag_DrawActiveEffects)
                {
                    Vec4f32 shadow_color = drop_shadow_color;
                    shadow_color.w *= 0.5 * box->active_t;
                    Vec2f32 shadow_size = {
                        (box_bg_rect.x1 - box_bg_rect.x0) * 0.60f * box->active_t,
                        (box_bg_rect.y1 - box_bg_rect.y0) * 0.60f * box->active_t,
                    };
                    shadow_size.x = Clamp(0, shadow_size.x, box->font_size * 2.f);
                    shadow_size.y = Clamp(0, shadow_size.y, box->font_size * 2.f);

                    // top -> bottom dark effect
                    {
                        R_Rect2D_Inst *inst = dr_rect(r2f32p(box_bg_rect.x0,
                                                             box_bg_rect.y0,
                                                             box_bg_rect.x1,
                                                             box_bg_rect.y0 + shadow_size.y),
                                                      v4f32(0, 0, 0, 0),
                                                      0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
                        MemoryCopyArray(inst->corner_radii, box_corner_radii);
                    }
                    
                    // rjf: bottom -> top light effect
                    {
                        R_Rect2D_Inst *inst = dr_rect(r2f32p(box_bg_rect.x0,
                                                             box_bg_rect.y1 - shadow_size.y,
                                                             box_bg_rect.x1,
                                                             box_bg_rect.y1),
                                                      v4f32(0, 0, 0, 0),
                                                      0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(1.0f, 1.0f, 1.0f, 0.08f*box->active_t);
                        MemoryCopyArray(inst->corner_radii, box_corner_radii);
                    }
          
                    // rjf: left -> right dark effect
                    {
                        R_Rect2D_Inst *inst = dr_rect(r2f32p(box_bg_rect.x0,
                                                             box_bg_rect.y0,
                                                             box_bg_rect.x0 + shadow_size.x,
                                                             box_bg_rect.y1),
                                                      v4f32(0, 0, 0, 0),
                                                      0, 0, 1.f);
                        inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_00] = shadow_color;
                        inst->colors[Corner_01] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box_corner_radii);
                    }
                    
                    // rjf: right -> left dark effect
                    {
                        R_Rect2D_Inst *inst = dr_rect(r2f32p(box_bg_rect.x1 - shadow_size.x,
                                                             box_bg_rect.y0,
                                                             box_bg_rect.x1,
                                                             box_bg_rect.y1),
                                                      v4f32(0, 0, 0, 0),
                                                      0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_11] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box_corner_radii);
                    }
                }
            }

            // draw string
            if(box->flags & UI_BoxFlag_DrawText)
            {
                Vec2f32 text_position = ui_box_text_position(box);
                if(DEV_draw_ui_text_pos)
                {
                    dr_rect(r2f32p(text_position.x-4,
                                   text_position.y-4,
                                   text_position.x+4,
                                   text_position.y+4),
                            v4f32(1, 0, 1, 1), 1, 0, 1);
                }
                f32 max_x = 100000.f;
                FNT_Run ellipses_run = {0};
                if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
                {
                    FNT_Tag ellipses_font = box->font;
                    f32 ellipses_size = box->font_size;
                    FNT_RasterFlags ellipses_raster_flags = box->text_raster_flags;
                    if(box->display_fstrs.last)
                    {
                        ellipses_font = box->display_fstrs.last->v.params.font;
                        ellipses_size = box->display_fstrs.last->v.params.size;
                        ellipses_raster_flags = box->display_fstrs.last->v.params.raster_flags;
                    }
                    max_x = (box->rect.x1-text_position.x);
                    ellipses_run = fnt_run_from_string(ellipses_font,
                                                       ellipses_size,
                                                       0,
                                                       box->tab_size,
                                                       ellipses_raster_flags,
                                                       str8_lit("..."));
                }
                if(box->flags & UI_BoxFlag_HasFuzzyMatchRanges) UI_TagF("match")
                {
                    Vec4f32 match_color = ui_color_from_tags_key_name(ui_top_tags_key(), str8_lit("background"));
                    dr_truncated_fancy_run_fuzzy_matches(text_position,
                                                         &box->display_fruns,
                                                         max_x,
                                                         &box->fuzzy_match_ranges,
                                                         match_color);
                }
                dr_truncated_fancy_run_list(text_position, &box->display_fruns, max_x, ellipses_run);
            }

            // draw focus viz
            if(DEV_draw_ui_focus_debug)
            {
                bool32 focused = (box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive) &&
                                  box->flags & UI_BoxFlag_Clickable);
                bool32 disabled = 0;
                for(UI_Box *p = box; !ui_box_is_nil(p); p = p->parent)
                {
                    if(p->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                    {
                        disabled = 1;
                        break;
                    }
                }
                if(focused)
                {
                    Vec4f32 color = v4f32(0.3f, 0.8f, 0.3f, 1.f);
                    if(disabled)
                    {
                        color = v4f32(0.8f, 0.3f, 0.3f, 1.f);
                    }
                    dr_rect(r2f32p(box->rect.x0-6,
                                   box->rect.y0-6,
                                   box->rect.x0+6,
                                   box->rect.y0+6),
                            color, 2, 0, 1);
                    dr_rect(box->rect, color, 2, 2, 1);
                }
                if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
                {
                    if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                    {
                        dr_rect(r2f32p(box->rect.x0-6,
                                       box->rect.y0-6,
                                       box->rect.x0+6,
                                       box->rect.y0+6),
                                v4f32(1, 0, 0, 0.2f), 2, 0, 1);
                    }
                    else
                    {
                        dr_rect(r2f32p(box->rect.x0-6,
                                       box->rect.y0-6,
                                       box->rect.x0+6,
                                       box->rect.y0+6),
                                v4f32(0, 1, 0, 0.2f),
                                2, 0, 1);
                    }
                }
            }

            // rjf: push clip
            if(box->flags & UI_BoxFlag_Clip)
            {
                Rng2f32 top_clip = dr_top_clip();
                Rng2f32 new_clip = pad_2f32(box->rect, -1);
                if(top_clip.x1 != 0 || top_clip.y1 != 0)
                {
                    new_clip = intersect_2f32(new_clip, top_clip);
                }
                dr_push_clip(new_clip);
            }
            
            // rjf: custom draw list
            if(box->flags & UI_BoxFlag_DrawBucket)
            {
                Mat3x3f32 xform = make_translate_3x3f32(box->position_delta);
                DR_XForm2DScope(xform)
                {
                    dr_sub_bucket(box->draw_bucket);
                }
            }
            
            // rjf: call custom draw callback
            if(box->custom_draw != 0)
            {
                box->custom_draw(box, box->custom_draw_user_data);
            }
            
            // rjf: pop
            {
                s32 pop_idx = 0;
                for(UI_Box *b = box; !ui_box_is_nil(b) && pop_idx <= rec.pop_count; b = b->parent)
                {
                    pop_idx += 1;
                    if(b == box && rec.push_count != 0)
                    {
                        continue;
                    }
                    
                    // rjf: pop clips
                    if(b->flags & UI_BoxFlag_Clip)
                    {
                        dr_pop_clip();
                    }
                    
                    // rjf: get corner radii
                    f32 b_corner_radii[Corner_COUNT] =
                        {
                            b->corner_radii[Corner_00]*rounded_corner_amount,
                            b->corner_radii[Corner_01]*rounded_corner_amount,
                            b->corner_radii[Corner_10]*rounded_corner_amount,
                            b->corner_radii[Corner_11]*rounded_corner_amount,
                        };
                    
                    // rjf: draw border
                    if(b->flags & UI_BoxFlag_DrawBorder)
                    {
                        Vec4f32 border_color = b->border_color;
                        Rng2f32 b_border_rect = r2f32p(b->rect.x0 - 1.f + b->visual_margin.x,
                                                       b->rect.y0 - 1.f + b->visual_margin.y,
                                                       b->rect.x1 + 1.f - b->visual_margin.x,
                                                       b->rect.y1 + 1.f - b->visual_margin.y);
                        R_Rect2D_Inst *inst = dr_rect(b_border_rect, border_color, 0, 1.f, border_softness*1.f);
                        MemoryCopyArray(inst->corner_radii, b_corner_radii);
                        
                        // rjf: hover effect
                        if(b->flags & UI_BoxFlag_DrawHotEffects)
                        {
                            Vec4f32 color = ui_color_from_tags_key_name(box->tags_key, str8_lit("hover"));
                            if(ui_key_match(b->key, ui_key_zero()) || !ui_key_match(b->key, ui_hot_key()))
                            {
                                color.w *= b->hot_t;
                            }
                            color.w *= 0.01f;
                            R_Rect2D_Inst *inst = dr_rect(b_border_rect, color, 0, 1.f, 1.f);
                            MemoryCopyArray(inst->corner_radii, b_corner_radii);
                        }
                    }
                    
                    // rjf: draw scroll fade
                    if(b->flags & (UI_BoxFlag_DrawFadeTop|UI_BoxFlag_DrawFadeBottom|UI_BoxFlag_DrawFadeLeft|UI_BoxFlag_DrawFadeRight))
                    {
                        Vec2f32 fade_dim = scale_2f32(dim_2f32(b->rect), 0.05f);
                        Vec4f32 fade_color = drop_shadow_color;
                        if(b->flags & UI_BoxFlag_DrawFadeTop)
                        {
                            f32 t = ui_anim(ui_key_from_string(b->key, s("fade_top")), 1.f, .rate = ti_state->catchall_animation_rate);
                            Rng2f32 rect = r2f32p(b->rect.x0, b->rect.y0, b->rect.x1, b->rect.y0 + fade_dim.y*t);
                            R_Rect2D_Inst *r = dr_rect(rect, fade_color, 0, 0, 0);
                            MemoryCopyArray(r->corner_radii, b_corner_radii);
                            r->colors[Corner_01] = r->colors[Corner_11] = v4f32(0, 0, 0, 0);
                        }
                        if(b->flags & UI_BoxFlag_DrawFadeBottom)
                        {
                            f32 t = ui_anim(ui_key_from_string(b->key, s("fade_bottom")), 1.f, .rate = ti_state->catchall_animation_rate);
                            Rng2f32 rect = r2f32p(b->rect.x0, b->rect.y1 - fade_dim.y*t, b->rect.x1, b->rect.y1);
                            R_Rect2D_Inst *r = dr_rect(rect, fade_color, 0, 0, 0);
                            MemoryCopyArray(r->corner_radii, b_corner_radii);
                            r->colors[Corner_00] = r->colors[Corner_10] = v4f32(0, 0, 0, 0);
                        }
                        if(b->flags & UI_BoxFlag_DrawFadeLeft)
                        {
                            f32 t = ui_anim(ui_key_from_string(b->key, s("fade_left")), 1.f, .rate = ti_state->catchall_animation_rate);
                            Rng2f32 rect = r2f32p(b->rect.x0, b->rect.y0, b->rect.x1+fade_dim.x*t, b->rect.y1);
                            R_Rect2D_Inst *r = dr_rect(rect, fade_color, 0, 0, 0);
                            MemoryCopyArray(r->corner_radii, b_corner_radii);
                            r->colors[Corner_11] = r->colors[Corner_10] = v4f32(0, 0, 0, 0);
                        }
                        if(b->flags & UI_BoxFlag_DrawFadeRight)
                        {
                            f32 t = ui_anim(ui_key_from_string(b->key, s("fade_right")), 1.f, .rate = ti_state->catchall_animation_rate);
                            Rng2f32 rect = r2f32p(b->rect.x1 - fade_dim.x*t, b->rect.y0, b->rect.x1, b->rect.y1);
                            R_Rect2D_Inst *r = dr_rect(rect, fade_color, 0, 0, 0);
                            MemoryCopyArray(r->corner_radii, b_corner_radii);
                            r->colors[Corner_00] = r->colors[Corner_01] = v4f32(0, 0, 0, 0);
                        }
                    }
                    
                    // rjf: debug border rendering
                    if(b->flags & UI_BoxFlag_Debug)
                    {
                        R_Rect2D_Inst *inst = dr_rect(b->rect, v4f32(1*box->pref_size[Axis2_X].strictness, 0, 1, 0.25f), 0, 1.f, 0);
                        MemoryCopyArray(inst->corner_radii, b_corner_radii);
                    }
                    
                    // rjf: draw sides
                    if(b->flags & (UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight))
                    {
                        Vec4f32 border_color = b->border_color;
                        Rng2f32 r = b->rect;
                        f32 half_thickness = 1.f;
                        f32 softness = 0.f;
                        if(b->flags & UI_BoxFlag_DrawSideTop)
                        {
                            dr_rect(r2f32p(r.x0, r.y0, r.x1, r.y0+2*half_thickness), border_color, 0, 0, softness);
                        }
                        if(b->flags & UI_BoxFlag_DrawSideBottom)
                        {
                            dr_rect(r2f32p(r.x0, r.y1-2*half_thickness, r.x1, r.y1), border_color, 0, 0, softness);
                        }
                        if(b->flags & UI_BoxFlag_DrawSideLeft)
                        {
                            dr_rect(r2f32p(r.x0, r.y0, r.x0+2*half_thickness, r.y1), border_color, 0, 0, softness);
                        }
                        if(b->flags & UI_BoxFlag_DrawSideRight)
                        {
                            dr_rect(r2f32p(r.x1-2*half_thickness, r.y0, r.x1, r.y1), border_color, 0, 0, softness);
                        }
                    }
                    
                    // rjf: draw focus overlay
                    if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
                    {
                        String8 extras[] = {str8_lit("focus"), str8_lit("overlay")};
                        String8_Array extras_array = {extras, ArrayCount(extras)};
                        Vec4f32 color = ui_color_from_tags_key_extras(b->tags_key, extras_array);
                        color.w *= b->focus_hot_t;
                        R_Rect2D_Inst *inst = dr_rect(b->rect, color, 0, 0, 0.f);
                        MemoryCopyArray(inst->corner_radii, b_corner_radii);
                    }
                    
                    // rjf: draw focus border
                    if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
                    {
                        Rng2f32 rect = b->rect;
                        if(b->flags & UI_BoxFlag_Floating)
                        {
                            rect = pad_2f32(rect, 1.f);
                            rect = intersect_2f32(window_rect, rect);
                        }
                        String8 extras[] = {str8_lit("focus"), str8_lit("border")};
                        String8_Array extras_array = {extras, ArrayCount(extras)};
                        Vec4f32 color = ui_color_from_tags_key_extras(b->tags_key, extras_array);
                        color.w *= b->focus_active_t;
                        R_Rect2D_Inst *inst = dr_rect(rect, color, 0, 1.f, border_softness*1.f);
                        MemoryCopyArray(inst->corner_radii, b_corner_radii);
                    }
                    
                    // rjf: disabled overlay
                    if(b->disabled_t >= 0.005f)
                    {
                        Vec4f32 disabled_overlay_color = v4f32(base_background_color.x, base_background_color.y, base_background_color.z, b->disabled_t*0.3f);
                        R_Rect2D_Inst *inst = dr_rect(b->rect, disabled_overlay_color, 0, 0, 1);
                        MemoryCopyArray(inst->corner_radii, b_corner_radii);
                    }
                    
                    // rjf: pop squish
                    if(b->squish > box_squish_epsilon)
                    {
                        dr_pop_xform2d();
                        dr_pop_tex2d_sample_kind();
                    }
                    
                    // rjf: pop transparency
                    if(b->transparency != 0)
                    {
                        dr_pop_transparency();
                    }
                }
            }

            // next
            box = rec.next;
        }

        //- rjf: draw heatmap
        if(DEV_draw_ui_box_heatmap)
        {
            u64 uniform_dist_count = total_heatmap_sum_count / heatmap_bucket_count;
            uniform_dist_count = ClampBot(uniform_dist_count, 10);
            for(u64 bucket_idx = 0; bucket_idx < heatmap_bucket_count; bucket_idx += 1)
            {
                u64 x = bucket_idx % heatmap_bucket_pitch;
                u64 y = bucket_idx / heatmap_bucket_pitch;
                u64 bucket = heatmap_buckets[bucket_idx];
                f32 pct = (f32)bucket / uniform_dist_count;
                pct = Clamp(0, pct, 1);
                Vec3f32 hsv = v3f32((1-pct) * 0.9411f, 1, 0.5f);
                Vec3f32 rgb = rgb_from_hsv(hsv);
                Rng2f32 rect = r2f32p(x*heatmap_bucket_size,
                                      y*heatmap_bucket_size,
                                      (x+1)*heatmap_bucket_size,
                                      (y+1)*heatmap_bucket_size);
                dr_rect(rect, v4f32(rgb.x, rgb.y, rgb.z, 0.3f), 0, 0, 0);
            }
        }

        // draw hover debug box
        if (hover_debug_box != &ui_nil_box)
        {
            FNT_Tag font = fnt_tag_from_static_data_string(&ti_default_main_font_bytes);
            Vec2f32 p = ui_mouse();
            dr_rect(hover_debug_box->rect, v4f32(1, 1, 1, 0.2f), 0, 0, 0);
            R_Rect2D_Inst *border = dr_rect(hover_debug_box->rect, v4f32(1, 0, 0, 1.f), 0, 0, 0);
            MemoryCopyArray(border->corner_radii, hover_debug_box->corner_radii);
            dr_text(font, 12.f, 0, 0, FNT_RasterFlag_Hinted, p, v4f32(1, 1, 1, 1), push_str8f(scratch.arena, "key: 0x%I64x", hover_debug_box->key.u64[0]));
            p.y += 20.f;
            dr_text(font, 12.f, 0, 0, FNT_RasterFlag_Hinted, p, v4f32(1, 1, 1, 1), push_str8f(scratch.arena, "string: '%S'", hover_debug_box->string));
            p.y += 20.f;
        }

        //- rjf: draw border/overlay color to signify error
        if(ws->error_t > 0.01f) UI_TagF("bad")
        {
            Vec4f32 color = ui_color_from_name(str8_lit("text"));
            color.w *= ws->error_t;
            Rng2f32 rect = wm_client_rect_from_window(ws->os);
            dr_rect(pad_2f32(rect, 24.f), color, 0, 16.f, 12.f);
            dr_rect(rect, v4f32(color.x, color.y, color.z, color.w*0.025f), 0, 0, 0);
        }

        scratch_end(scratch);
    }

    /////////////////////
    // @window_frame_part update per-window frame counters/info
    //
    ws->frames_alive += 1;
    ws->last_window_rect = wm_client_rect_from_window(ws->os);

    ProfEnd();
    scratch_end(scratch);
}

#if COMPILER_MSVC && !BUILD_DEBUG
NO_OPTIMIZE_END
#endif

//////////////////////////
// Colors, Fonts, Config

// colors

//fonts
internal f32 ti_font_size(void)
{
    // TODO make this real
    return 16.f;
}

//////////////////////////////
// Continuous Frame Requests

internal void ti_request_frame(void)
{
    ti_state->num_frames_requested = 4;
}

////////////////////////////
// Main State Accessors

// per-frame arena
internal Arena *ti_frame_arena(void)
{
    return ti_state->frame_arenas[ti_state->frame_index % ArrayCount(ti_state->frame_arenas)];
}

///////////////
// Registers

internal TI_Regs *ti_push_regs_(TI_Regs *regs)
{
    TI_Regs_Node *n = push_array(ti_frame_arena(), TI_Regs_Node, 1);
    ti_regs_copy_contents(ti_frame_arena(), &n->v, regs);
    SLLStackPush(ti_state->top_regs, n);
    return &n->v;
}

internal TI_Regs *ti_pop_regs(void)
{
    TI_Regs *regs = &ti_state->top_regs->v;
    SLLStackPop(ti_state->top_regs);
    if (ti_state->top_regs == 0)
    {
        ti_state->top_regs = &ti_state->base_regs;
    }
    return regs;
}

/////////////////////
// Commands

// name -> info
internal TI_CmdKind ti_cmd_kind_from_string(String8 string)
{
    TI_CmdKind result = TI_CmdKind_Null;
    for (u64 idx = 0; idx < ArrayCount(ti_cmd_kind_info_table); idx += 1)
    {
        if (str8_match(string, ti_cmd_kind_info_table[idx].string, 0))
        {
            result = (TI_CmdKind)idx;
            break;
        }
    }
    return result;
}

internal TI_Cmd_Kind_Info *ti_cmd_kind_info_from_string(String8 string)
{
    TI_Cmd_Kind_Info *info = &ti_nil_cmd_kind_info;
    {
        TI_CmdKind kind = ti_cmd_kind_from_string(string);
        if (kind != TI_CmdKind_Null)
        {
            info = &ti_cmd_kind_info_table[kind];
        }
    }
    return info;
}

// pushing

internal void ti_push_cmd(String8 name, TI_Regs *regs)
{
    ti_cmd_list_push_new(ti_state->cmds_arenas[0], &ti_state->cmds[0], name, regs);
}

// iterating

internal bool32 ti_next_cmd(TI_Cmd **cmd)
{
    u64 slot = ti_state->cmds_gen % ArrayCount(ti_state->cmds);
    TI_Cmd_Node *start_node = ti_state->cmds[slot].first;
    if (cmd[0] != 0)
    {
        start_node = CastFromMember(TI_Cmd_Node, cmd, cmd[0]);
        start_node = start_node->next;
    }
    cmd[0] = 0;
    if (start_node != 0)
    {
        cmd[0] = &start_node->cmd;
    }
    return !!cmd[0];
}

/*
internal bool32 ti_next_view_cmd(TI_Cmd **cmd)
{
    for (;ti_next_cmd(cmd);)
    {
        if (ti_regs()->view == cmd[0]->regs->view)
        {
            break;
        }
    }
    bool32 result = !!cmd[0];
    return result;
}
*/

//////////////////////////////
// Main Layer Top-Level Calls

#if !defined(STBI_INCLUDE_STB_IMAGE_H)
# define STB_IMAGE_IMPLEMENTATION
# define STBI_ONLY_PNG
# define STBI_ONLY_BMP
# include "third_party/stb/stb_image.h"
#endif

internal void ti_init(Cmd_Line *cmdline)
{
    ProfBeginFunction();
    Temp scratch = scratch_begin(0, 0);
    Arena *arena = arena_alloc();
    ti_state = push_array(arena, TI_State, 1);
    ti_state->arena = arena;
    for (u64 idx = 0; idx < ArrayCount(ti_state->frame_arenas); idx += 1)
    {
        ti_state->frame_arenas[idx] = arena_alloc();
    }
    ti_state->log = log_alloc();
    log_select(ti_state->log);
    {
        Temp scratch = scratch_begin(0, 0);
        ti_state->log_path = push_str8f(ti_state->arena, "%S/ui_thread.tide_log", g_logs_folder);
        write_data_to_file_path(ti_state->log_path, str8_zero());
        scratch_end(scratch);
    }
    ti_state->num_frames_requested = 2;
    for (u64 idx = 0; idx < ArrayCount(ti_state->cmds_arenas); idx += 1)
    {
        ti_state->cmds_arenas[idx] = arena_alloc();
    }
    ti_state->cmd_output_arena = arena_alloc();
    ti_state->top_regs = &ti_state->base_regs;

    // set up top-level config entity trees & tables
    {
        ti_state->cfg = cfg_state_alloc();
        cfg_ctx_select(cfg_state_ctx(ti_state->cfg));
        cfg_node_new(ti_state->cfg, cfg_node_root(), str8_lit("user"));
    }

    // set up window cache
    {
        ti_state->window_state_slots_count = 64;
        ti_state->window_state_slots = push_array(arena, TI_Window_State_Slot, ti_state->window_state_slots_count);
        ti_state->first_window_state = ti_state->last_window_state = &ti_nil_window_state;
    }

    // set up user / project paths
    {
        CFG_Node_Ptr_List all_user_windows = cfg_node_top_level_list_from_string(scratch.arena, str8_lit("window"));
        if (all_user_windows.count == 0)
        {
            WM_Monitor monitor   = wm_primary_monitor();
            String8 monitor_name = wm_name_from_monitor(scratch.arena, monitor);
            Vec2f32 monitor_dim  = wm_dim_from_monitor(monitor);
            f32 monitor_dpi      = wm_dpi_from_monitor(monitor);
            Vec2f32 window_dim   = v2f32(monitor_dim.x * 4/5, monitor_dim.y * 4/5);
            if (window_dim.x == 0 || window_dim.y == 0)
            {
                window_dim = v2f32(1280, 720);
            }
            CFG_Node *user = cfg_node_child_from_string(cfg_node_root(), str8_lit("user"));
            CFG_Node *new_window = cfg_node_new(ti_state->cfg, user, str8_lit("window"));
            CFG_Node *size = cfg_node_new(ti_state->cfg, new_window, str8_lit("size"));
            cfg_node_newf(ti_state->cfg, size, "%f", window_dim.x);
            cfg_node_newf(ti_state->cfg, size, "%f", window_dim.y);
        }
    }

    // set up world map
    {
    }

    ProfEnd();
    scratch_end(scratch);
}
                         
internal void ti_frame(void)
{
    ProfBeginFunction();
    Temp scratch = scratch_begin(0, 0);
    log_scope_begin();
    ti_state->frame_depth += 1;

    ///////////////////////
    // do per-frame resets
    {
        Temp scratch = scratch_begin(0, 0);
        ti_state->top_regs = &ti_state->base_regs;
        ti_regs_copy_contents(scratch.arena, &ti_state->top_regs->v, &ti_state->top_regs->v);
        arena_clear(ti_frame_arena());
        ti_regs_copy_contents(ti_frame_arena(), &ti_state->top_regs->v, &ti_state->top_regs->v);
        scratch_end(scratch);
    }
    if (ti_state->frame_depth == 1)
    {
        arena_clear(ti_state->cmd_output_arena);
        MemoryZeroStruct(&ti_state->cmd_outputs);
    }

    //////////////////////////////
    // iterate all tabs, touch their view states
    //
    if (ti_state->frame_depth == 1)
    {
        Temp scratch = scratch_begin(0, 0);
        CFG_Node_Ptr_List windows = cfg_node_top_level_list_from_string(scratch.arena, str8_lit("window"));
        // todo: finish
    }

    ////////////////////////////
    // get events from the OS
    //
    WM_Event_List events = {0};
    if (ti_state->frame_depth == 1)
    {
        events = wm_get_events(scratch.arena, ti_state->num_frames_requested == 0);// make a DEV_always_refresh here
    }

    ////////////////////
    // push frame scopes
    //
    Access *frame_access_restore = ti_state->frame_access;
    ti_state->frame_access = access_open();

    /////////////////////////////////////////////////
    // calculate avg length in us of last many frames
    //
    u64 frame_time_history_avg_us = 0;
    {
        u64 num_frames_in_history = Min(ArrayCount(ti_state->frame_time_us_history), ti_state->frame_index);
        u64 frame_time_history_sum_us = 0;
        if (num_frames_in_history > 0)
        {
            for (u64 idx = 0; idx < num_frames_in_history; idx += 1)
            {
                frame_time_history_sum_us += ti_state->frame_time_us_history[idx];
            }
            frame_time_history_avg_us = frame_time_history_sum_us / num_frames_in_history;
        }
    }
    
    ///////////////////////
    // pick target hz
    //
    // pick among number of sensible targets to snap to
    //
    f32 target_hz = wm_get_system_info()->default_refresh_rate;
    if (ti_state->frame_index > 32)
    {
        f32 possible_alternate_hz_targets[] = {target_hz, 60.f, 75.f, 120.f, 144.f, 165.f, 240.f, 360.f};
        f32 best_target_hz = target_hz;
        s64 best_target_hz_frame_time_us_diff = max_s64;
        for (u64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
        {
            f32 candidate = possible_alternate_hz_targets[idx];
            if (candidate <= target_hz)
            {
                u64 candidate_frame_time_us = 1000000 / (u64)candidate;
                s64 frame_time_us_diff = (s64)frame_time_history_avg_us - (s64)candidate_frame_time_us;
                if (abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff &&
                    frame_time_history_avg_us < candidate_frame_time_us + candidate_frame_time_us / 4)
                {
                    best_target_hz = candidate;
                    best_target_hz_frame_time_us_diff = frame_time_us_diff;
                }
            }
        }
        target_hz = best_target_hz;
    }

    //////////////////////////
    // target Hz -> delta time
    ti_state->frame_dt = 1.f / target_hz;

    /////////////////////////////////////////
    // megin measuring actual per-frame work
    u64 begin_time_us = now_time_us();

    ////////////////////
    // consume events
    ProfScope("consume events")
    {
        for (WM_Event *event = events.first, *next = 0;
             event != 0;
             event = next)
            TI_RegsScope()
        {
            next = event->next;
            TI_Window_State *ws = ti_window_state_from_os_handle(event->window);
            if (ws != 0 && ws != ti_window_state_from_cfg(cfg_node_from_id(ti_regs()->window)))
            {
                Temp scratch = scratch_begin(0, 0);
                CFG_Panel_Tree panel_tree = cfg_panel_tree_from_cfg(scratch.arena, cfg_node_from_id(ws->cfg_id));
                ti_regs()->window = ws->cfg_id;
                scratch_end(scratch);
            }
            bool32 take = false;

            // try window close
            if (!take && event->kind == WM_EventKind_WindowClose && ws != 0)
            {
                take = true;
                ti_cmd(TI_CmdKind_Exit);
            }

            // do fall-through
            if (!take)
            {
                take = true;
                ti_cmd(TI_CmdKind_WMEvent, .wm_event = event);
            }

            // take
            if (take)
            {
                wm_eat_event(&events, event);
            }
        }
    }

    ////////////////////////////////
    // loop - consume events in core, tick engine, and repeat
    //
    TI_Cmd *cmd = 0;
    ProfScope("loop - consume events in core, tick engine, and repeat") for (u64 cmd_process_loop_idx = 0; cmd_process_loop_idx < 3; cmd_process_loop_idx += 1)
    {
        /////////////////////////////
        // process top-level graphical commands
        //
        if (ti_state->frame_depth == 1) ProfScope("process top-level graphical commands")
        {
            for (;ti_next_cmd(&cmd);) TI_RegsScope()
            {
                // unpack command
                TI_CmdKind kind = ti_cmd_kind_from_string(cmd->name);
                ti_regs_copy_contents(ti_frame_arena(), ti_regs(), cmd->regs);

                // request frame
                ti_request_frame();

                // process command
                CFG_Node *cfg = &cfg_nil_node;
                switch (kind)
                {
                    case TI_CmdKind_Exit: {
                        {
                            ti_state->quit = true;
                        }
                    } break;
                    case TI_CmdKind_OpenWindow: {
                        {
                            CFG_Node *old_window = cfg_node_from_id(ti_regs()->window);
                            CFG_Node *bucket = old_window->parent;
                            if (bucket == &cfg_nil_node)
                            {
                                bucket = cfg_node_child_from_string(cfg_node_root(), str8_lit("user"));
                            }
                            CFG_Node *new_window = cfg_node_new(ti_state->cfg, bucket, str8_lit("window"));
                            CFG_Node *size = cfg_node_new(ti_state->cfg, new_window, str8_lit("size"));
                            cfg_node_newf(ti_state->cfg, size, "1280");
                            cfg_node_newf(ti_state->cfg, size, "720");
                            for (CFG_Node *old_child = old_window->first;
                                 old_child != &cfg_nil_node;
                                 old_child = old_child->next)
                            {
                                if (!str8_match(old_child->string, str8_lit("panels"), 0) &&
                                    !str8_match(old_child->string, str8_lit("size"), 0) &&
                                    !str8_match(old_child->string, str8_lit("pos"), 0) &&
                                    !str8_match(old_child->string, str8_lit("monitor"), 0) &&
                                    !str8_match(old_child->string, str8_lit("fullscreen"), 0) &&
                                    !str8_match(old_child->string, str8_lit("maximized"), 0))
                                {
                                    CFG_Node *new_child = cfg_node_deep_copy(ti_state->cfg, old_child);
                                    cfg_node_insert_child(ti_state->cfg, new_window, new_window->last, new_child);
                                }
                            }
                            CFG_Node *panels = cfg_node_new(ti_state->cfg, new_window, str8_lit("panels"));
                            cfg_node_child_from_string_or_alloc(ti_state->cfg, panels, str8_lit("selected"));
                        }
                    } break;
                    case TI_CmdKind_CloseWindow: {
                        {
                            CFG_Node_Ptr_List all_windows = cfg_node_top_level_list_from_string(scratch.arena, str8_lit("window"));
                            CFG_Node *wcfg = cfg_node_from_id(ti_regs()->window);
                            if (all_windows.count == 1 && all_windows.first->v == wcfg)
                            {
                                ti_cmd(TI_CmdKind_Exit);
                            }
                            else
                            {
                                cfg_node_release(ti_state->cfg, wcfg);
                            }
                        }
                    } break;
                    case TI_CmdKind_WMEvent: {
                        {
                            WM_Event *wm_event = ti_regs()->wm_event;
                            TI_Window_State *ws = ti_window_state_from_os_handle(wm_event->window);
                            if (wm_event != 0 && ws != &ti_nil_window_state)
                            {
                                UI_Event ui_event = zero_struct;
                                UI_EventKind kind = UI_EventKind_Null;
                                {
                                    switch (wm_event->kind)
                                    {
                                        default:{}break;
                                        case WM_EventKind_Press:     {kind = UI_EventKind_Press;}break;
                                        case WM_EventKind_Release:   {kind = UI_EventKind_Release;}break;
                                        case WM_EventKind_MouseMove: {kind = UI_EventKind_MouseMove;}break;
                                        case WM_EventKind_Text:      {kind = UI_EventKind_Text;}break;
                                        case WM_EventKind_Scroll:    {kind = UI_EventKind_Scroll;}break;
                                        case WM_EventKind_FileDrop:  {kind = UI_EventKind_FileDrop;}break;
                                    }
                                }
                                ui_event.kind         = kind;
                                ui_event.key          = wm_event->key;
                                ui_event.modifiers    = wm_event->modifiers;
                                ui_event.string       = wm_event->character ? str8_from_32(ui_build_arena(), str32(&wm_event->character, 1)) : str8_zero();
                                ui_event.paths        = str8_list_copy(ui_build_arena(), &wm_event->strings);
                                ui_event.pos          = wm_event->pos;
                                ui_event.delta_2f32   = wm_event->delta;
                                ui_event.timestamp_us = wm_event->timestamp_us;
                                ui_event_list_push(scratch.arena, &ws->ui_events, &ui_event);
                            }
                        }
                    } break;
                }
            }
        }

        //////////////
        // tick measurement engine
        // TODO
        u64 cmd_count_pre_tick = ti_state->cmds[0].count;

        //////////////////////////
        // process measurement engine events
        // TODO

        
        /////////////////////////
        // early out if no new commands
        //
        if (ti_state->cmds[0].count == cmd_count_pre_tick)
        {
            break;
        }
    }

    //////////////////////////
    // update window titles
    if (ti_state->frame_depth == 1)
    {
        Temp scratch = scratch_begin(0, 0);
        String8 window_title = ti_push_window_title(scratch.arena);
        if (!str8_match(window_title, ti_state->last_window_title, 0))
        {
            for (TI_Window_State *ws = ti_state->first_window_state; ws != &ti_nil_window_state; ws = ws->order_next)
            {
                wm_window_set_title(ws->os, window_title);
            }
        }
        ti_state->last_window_title = str8_copy(ti_frame_arena(), window_title);
        scratch_end(scratch);
    }

    ////////////////////////////
    //- rjf: rotate command slots, bump command gen counter
    //
    // in this step, we rotate the ring buffer of command batches (command
    // arenas & lists). when the cmds_gen (the position of the ring buffer)
    // is even, the command queue is in a "read/write" mode, and this is uniquely
    // usable by the core - this is done so that commands in the core can push
    // other commands, and have those other commands processed on the same frame.
    //
    // in view code, however, they can only use the current command queue in a
    // "read only" mode, because new commands pushed by those views must be
    // processed first by the core. so, before calling into view code, the
    // cmds_gen is incremented to be *odd*. this way, the views will *write*
    // commands into the 0 slot, but *read* from the 1 slot (which will contain
    // this frame's commands).
    //
    // after view code runs, the generation number is incremented back to even.
    // the commands pushed by the view will be in the queue, and the core can
    // treat that queue as r/w again.
    //
    if (ti_state->frame_depth == 1)
    {
        // rotate
        {
            Arena *first_arena = ti_state->cmds_arenas[0];
            TI_Cmd_List first_cmds = ti_state->cmds[0];
            MemoryCopy(ti_state->cmds_arenas,
                       ti_state->cmds_arenas + 1,
                       sizeof(ti_state->cmds_arenas[0]) * (ArrayCount(ti_state->cmds_arenas) - 1));
            MemoryCopy(ti_state->cmds,
                       ti_state->cmds + 1,
                       sizeof(ti_state->cmds[0]) * (ArrayCount(ti_state->cmds) - 1));
            ti_state->cmds_arenas[ArrayCount(ti_state->cmds_arenas) - 1] = first_arena;
            ti_state->cmds[ArrayCount(ti_state->cmds_arenas) - 1] = first_cmds;
        }

        // clear next batch
        {
            arena_clear(ti_state->cmds_arenas[0]);
            MemoryZeroStruct(&ti_state->cmds[0]);
        }

        // bump
        {
            ti_state->cmds_gen += 1;
        }
    }

    ////////////////////////////////////
    // compute amimation rates, given config
    //
    {
        f32 master_animations_f    = 1.f;
        f32 scrolling_animations_f = 1.f;
        f32 tooltip_animations_f   = 1.f;
        f32 menu_animations_f      = 1.f;

        ti_state->catchall_animation_rate     = 1 - master_animations_f*pow_f32(2, (-60.f * ti_state->frame_dt));
        ti_state->menu_animation_rate         = 1 - master_animations_f*menu_animations_f*pow_f32(2, (-70.f * ti_state->frame_dt));
        ti_state->menu_animation_rate__slow   = 1 - master_animations_f*menu_animations_f*pow_f32(2, (-50.f * ti_state->frame_dt));
        ti_state->entity_alive_animation_rate = 1 - master_animations_f*menu_animations_f*pow_f32(2, (-30.f * ti_state->frame_dt));
        ti_state->rich_hover_animation_rate   = 1 - master_animations_f*menu_animations_f*pow_f32(2, (-50.f * ti_state->frame_dt));
        ti_state->scrolling_animation_rate    = 1 - master_animations_f*scrolling_animations_f*pow_f32(2, (-60.f * ti_state->frame_dt));
        ti_state->tooltip_animation_rate      = 1 - master_animations_f*tooltip_animations_f*pow_f32(2, (-60.f * ti_state->frame_dt));
    }

    ///////////////////////////////
    // update/render all windows
    {
        dr_begin_frame(fnt_tag_from_static_data_string(&ti_default_main_font_bytes));
        CFG_Node_Ptr_List windows = cfg_node_top_level_list_from_string(scratch.arena, str8_lit("window"));
        for (CFG_Node_Ptr_Node *n = windows.first; n != 0; n = n->next)
        {
            CFG_Node *window = n->v;
            TI_Window_State *w = ti_window_state_from_cfg(window);
            bool32 window_is_focused = wm_window_is_focused(w->os);
            if (window_is_focused)
            {
                ti_state->last_focused_window = w->cfg_id;
            }
            ti_push_regs();
            ti_regs()->window = w->cfg_id;
            ti_window_frame();
            MemoryZeroStruct(&w->ui_events);
            TI_Regs *window_regs = ti_pop_regs();
            if (ti_state->last_focused_window == w->cfg_id)
            {
                MemoryCopyStruct(ti_regs(), window_regs);
            }
        }
    }

    /////////////////////////////////
    // garbage collect untouched window states
    //
    {
        for EachIndex(slot_idx, ti_state->window_state_slots_count)
        {
            for (TI_Window_State *ws = ti_state->window_state_slots[slot_idx].first, *next = 0; ws != 0; ws = next)
            {
                next = ws->hash_next;
                CFG_Node *cfg = cfg_node_from_id(ws->cfg_id);
                if (cfg == &cfg_nil_node || ws->last_frame_index_touched < ti_state->frame_index || ti_state->quit)
                {
                    ui_state_release(ws->ui);
                    r_window_unequip(ws->os, ws->r);
                    wm_window_close(ws->os);
                    arena_release(ws->arena);
                    DLLRemove_NPZ(&ti_nil_window_state, ti_state->first_window_state, ti_state->last_window_state, ws, order_next, order_prev);
                    DLLRemove_NP(ti_state->window_state_slots[slot_idx].first, ti_state->window_state_slots[slot_idx].last, ws, hash_next, hash_prev);
                    SLLStackPush_N(ti_state->free_window_state, ws, order_next);
                }
            }
        }
    }

    //////////////////////////////
    // clear frame request state
    if (ti_state->num_frames_requested > 0)
    {
        ti_state->num_frames_requested -= 1;
    }

    //////////////////////////
    // close frame scopes
    //
    // NOTE: this always must happen before the refresh, since that
    // will sleep for vsync, and we do not want to hold handles for long,
    // since eviction threads may be wariing to get rid of stuff.
    //
    access_close(ti_state->frame_access);
    ti_state->frame_access = frame_access_restore;

    /////////////////////////////
    // submit rendering to all windows
    ProfScope("submit rendering to all windows")
    {
        r_begin_frame();
        for (TI_Window_State *w = ti_state->first_window_state; w != &ti_nil_window_state; w = w->order_next)
        {
            r_window_begin_frame(w->os, w->r);
            dr_submit_bucket(w->os, w->r, w->draw_bucket);
            r_window_end_frame(w->os, w->r);
        }
        r_end_frame();
    }

    //////////////////////////
    // show windows after first frame
    if (ti_state->frame_depth == 1)
    {
        CFG_ID_List windows_to_show = {0};
        for (TI_Window_State *w = ti_state->first_window_state; w != &ti_nil_window_state; w = w->order_next)
        {
            if (w->frames_alive == 1)
            {
                cfg_id_list_push(scratch.arena, &windows_to_show, w->cfg_id);
            }
        }
        for (CFG_ID_Node *n = windows_to_show.first; n != 0; n = n->next)
        {
            CFG_Node *window = cfg_node_from_id(n->v);
            TI_Window_State *ws = ti_window_state_from_cfg(window);
            wm_window_first_paint(ws->os);
        }
    }

    //////////////////////////////////////
    // determine frame time, record into history
    u64 end_time_us = now_time_us();
    u64 frame_time_us = end_time_us - begin_time_us;
    ti_state->frame_time_us_history[ti_state->frame_index % ArrayCount(ti_state->frame_time_us_history)] = frame_time_us;

    ////////////////////////////
    // [windows] clear pages from working set shortly after startup, many of which will not be needed
#if OS_WINDOWS
    if (ti_state->frame_index == 15) ProfScope("SetProcessWorkingSetSize")
    {
        SetProcessWorkingSetSize(GetCurrentProcess(), max_u64, max_u64);
    }
#endif

    ///////////////////////////
    // bump frame time counters
    //
    ti_state->frame_index += 1;
    ti_state->time_in_seconds += ti_state->frame_dt;
    ti_state->time_in_us += frame_time_us;

    //////////////////////////
    // bump command batch ring buffer generation
    //
    if (ti_state->frame_depth == 1)
    {
        ti_state->cmds_gen += 1;
    }

    /////////////////
    // collect logs
    ProfScope("collect logs")
    {
        Log_Scope_Result log = log_scope_end(scratch.arena);
        append_data_to_file_path(ti_state->log_path, log.strings[LogMsgKind_Info]);
        if (log.strings[LogMsgKind_UserError].size != 0)
        {
            String8 error_log = log.strings[LogMsgKind_UserError];
            String8_List error_log_lines = str8_split(scratch.arena, error_log, (u8 *)"\n", 1, 0);
            String8 error_log_string = str8_list_join(scratch.arena,
                                                      &error_log_lines,
                                                      &(String_Join){.sep = str8_lit(" ")});
            for (TI_Window_State *ws = ti_state->first_window_state; ws != &ti_nil_window_state; ws = ws->order_next)
            {
                ws->error_string_size = Min(sizeof(ws->error_buffer), error_log_string.size);
                MemoryCopy(ws->error_buffer, error_log_string.str, ws->error_string_size);
                ws->error_t = 1.f;
            }
        }
    }

    ti_state->frame_depth -= 1;
    scratch_end(scratch);
    ProfEnd();
}

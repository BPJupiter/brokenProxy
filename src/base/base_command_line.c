
/////////////////////////////////////
// Command Line Parsing Functions

internal Cmd_Line_Opt **cmd_line_slot_from_string(Cmd_Line *cmdline, String8 string)
{
    Cmd_Line_Opt **slot = 0;
    if (cmdline->option_table_size != 0) {
        u64 hash = u64_hash_from_str8(string);
        u64 bucket = hash % cmdline->option_table_size;
        slot = &cmdline->option_table[bucket];
    }
    return slot;
}

internal Cmd_Line_Opt *cmd_line_opt_from_slot(Cmd_Line_Opt **slot, String8 string)
{
    Cmd_Line_Opt *result = 0;
    for (Cmd_Line_Opt *var = *slot; var; var = var->hash_next) {
        if (str8_match(string, var->string, 0)) {
            result = var;
            break;
        }
    }
    return result;
}

internal void cmd_line_push_opt(Cmd_Line_Opt_List *list, Cmd_Line_Opt *var)
{
    SLLQueuePush(list->first, list->last, var);
    list->count += 1;
}

internal Cmd_Line_Opt *cmd_line_insert_opt(Arena *arena, Cmd_Line *cmdline, String8 string, String8_List values)
{
    Cmd_Line_Opt *var = 0;
    Cmd_Line_Opt **slot = cmd_line_slot_from_string(cmdline, string);
    Cmd_Line_Opt *existing_var = cmd_line_opt_from_slot(slot, string);
    if (existing_var) {
        var = existing_var;
    }
    else {
        var = push_array(arena, Cmd_Line_Opt, 1);
        var->hash_next = *slot;
        var->hash = u64_hash_from_str8(string);
        var->string = push_str8_copy(arena, string);
        var->value_strings = values;
        String_Join join = {0};
        join.pre = str8_lit("");
        join.sep = str8_lit(",");
        join.post = str8_lit("");
        var->value_string = str8_list_join(arena, &var->value_strings, &join);
        *slot = var;
        cmd_line_push_opt(&cmdline->options, var);
    }
    return var;
}

internal Cmd_Line cmd_line_from_string_list(Arena *arena, String8_List command_line)
{
    Cmd_Line parsed = {0};
    parsed.exe_name = command_line.first->string;
    parsed.option_table_size = 64;
    parsed.option_table = push_array(arena, Cmd_Line_Opt *, parsed.option_table_size);

    // parse options / inputs
    bool32 after_passthrough_option = false;
    bool32 first_passthrough = true;
    for (String8_Node *node = command_line.first->next, *next = 0; node != 0; node = next) {
        next = node->next;

        // Look at --, -, or / (only on windows) at the start of an
        // argument to determine if its a flag option. all arguments
        // after a single '--' (with no trailing string on the command line) will
        // be considered as passthrough input strings
        bool32 is_option = false;
        String8 option_name = node->string;
        if (!after_passthrough_option) {
            is_option = true;
            if (str8_match(node->string, str8_lit("--"), 0)) {
                after_passthrough_option = true;
                is_option = false;
            }
            else if (str8_match(str8_prefix(node->string, 2), str8_lit("--"), 0)) {
                option_name = str8_skip(option_name, 2);
            }
            else if (str8_match(str8_prefix(node->string, 1), str8_lit("-"), 0)) {
                option_name = str8_skip(option_name, 1);
            }
            else if (OperatingSystem_CURRENT == OperatingSystem_Windows &&
                     str8_match(str8_prefix(node->string, 1), str8_lit("/"), 0)) {
                option_name = str8_skip(option_name, 1);
            }
            else {
                is_option = false;
            }
        }

        if (is_option) {
            bool32 has_values = false;
            u64 value_signifier_position1 = str8_find_needle(option_name, 0, str8_lit(":"), 0);
            u64 value_signifier_position2 = str8_find_needle(option_name, 0, str8_lit("="), 0);
            u64 value_signifier_position = Min(value_signifier_position1, value_signifier_position2);
            String8 value_portion_this_string = str8_skip(option_name, value_signifier_position + 1);
            if (value_signifier_position < option_name.size) {
                has_values = true;
            }
            option_name = str8_prefix(option_name, value_signifier_position);

            // parse option's values
            String8_List values = {0};
            if (has_values) {
                for (String8_Node *n = node; n; n = n->next) {
                    next = n->next;
                    String8 string = n->string;
                    if (n == node) {
                        string = value_portion_this_string;
                    }
                    u8 splits[] = { ',' };
                    String8_List values_in_this_string = str8_split(arena, string, splits, ArrayCount(splits), 0);
                    for (String8_Node *sub_val = values_in_this_string.first; sub_val; sub_val = sub_val->next) {
                        str8_list_push(arena, &values, sub_val->string);
                    }

                    if (!str8_match(str8_postfix(n->string, 1), str8_lit(","), 0) &&
                        (n != node || value_portion_this_string.size != 0)) {
                        break;
                    }
                }
            }

            cmd_line_insert_opt(arena, &parsed, option_name, values);
        }

        else if (!str8_match(node->string, str8_lit("--"), 0) || !first_passthrough) {
            str8_list_push(arena, &parsed.inputs, node->string);
            first_passthrough = false;
        }
    }

    // fill argc, argv
    parsed.argc = command_line.node_count;
    parsed.argv = push_array(arena, char *, parsed.argc);
    {
        u64 idx = 0;
        for (String8_Node *n = command_line.first; n != 0; n = n->next) {
            parsed.argv[idx] = (char *)push_str8_copy(arena, n->string).str;
            idx += 1;
        }
    }

    return parsed;
}

internal Cmd_Line_Opt *cmd_line_opt_from_string(Cmd_Line *cmdline, String8 name)
{
    return cmd_line_opt_from_slot(cmd_line_slot_from_string(cmdline, name), name);
}

internal String8_List cmd_line_strings(Cmd_Line *cmdline, String8 name)
{
    String8_List result = {0};
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmdline, name);
    if (var != 0) {
        result = var->value_strings;
    }
    return result;
}

internal String8 cmd_line_string(Cmd_Line *cmdline, String8 name)
{
    String8 result = {0};
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmdline, name);
    if (var != 0) {
        result = var->value_string;
    }
    return result;
}

internal bool32 cmd_line_has_flag(Cmd_Line *cmdline, String8 name)
{
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmdline, name);
    return (var != 0);
}

internal bool32 cmd_line_has_argument(Cmd_Line *cmdline, String8 name)
{
    Cmd_Line_Opt *var = cmd_line_opt_from_string(cmdline, name);
    return (var != 0 && var->value_strings.node_count > 0);
}


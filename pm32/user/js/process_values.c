/* Included by js.c: native ProcessInfo array/object projection. */
static int process_property(ProcessInfo *process, const char *field,
                            long *number, const char **text)
{
    *text = 0;
    if (text_equal(field, "pid")) *number = (long)process->pid;
    else if (text_equal(field, "name")) *text = process->name;
    else if (text_equal(field, "active")) *number = (long)process->active;
    else if (text_equal(field, "state"))
        *text = process->active ? "RUN" : "WAIT";
    else if (text_equal(field, "type"))
        *text = process->app_type == APP_GUI ? "GUI" : "CONSOLE";
    else if (text_equal(field, "appType")) *number = (long)process->app_type;
    else if (text_equal(field, "parentPid")) *number = (long)process->parent_pid;
    else if (text_equal(field, "terminalPid")) *number = (long)process->terminal_pid;
    else {
        error_text = "unknown process property";
        return 0;
    }
    return 1;
}

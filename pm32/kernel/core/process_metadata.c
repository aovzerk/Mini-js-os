/* Included by core.c: process metadata shared by exec, spawn and syscall 22. */
static void set_process_name(unsigned pid, const char name[11])
{
    unsigned i;
    unsigned length = 8;

    while (length && name[length - 1] == ' ') --length;
    for (i = 0; i < 9; ++i) processes[pid].name[i] = 0;
    for (i = 0; i < length; ++i) processes[pid].name[i] = name[i];
    for (i = 0; i < 32; ++i) processes[pid].gui_title[i] = 0;
    for (i = 0; i < length; ++i) processes[pid].gui_title[i] = name[i];
}

static void fill_process_info(ProcessInfo *info, unsigned pid)
{
    unsigned i;

    info->pid = pid;
    for (i = 0; i < 9; ++i) info->name[i] = processes[pid].name[i];
    info->active = processes[pid].active;
    info->app_type = processes[pid].app_type;
    info->parent_pid = processes[pid].parent_pid;
    info->terminal_pid = processes[pid].terminal_pid;
}

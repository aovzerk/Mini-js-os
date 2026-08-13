static u8 *user_memory(void)
{
    volatile u32 twice_user_base=0x40000UL;
    return (u8 *)(twice_user_base>>1);
}

static void make_address_space(unsigned pid)
{
    u32 *pd=(u32 *)(PAGE_DIRECTORY_BASE+pid*4096UL);
    u32 *pt = (u32 *)(PAGE_TABLE_BASE + pid * 4096UL);
    unsigned i;

    for (i = 0; i < 1024; ++i) {
        pd[i] = (i << 22) | 0x87;
        pt[i] = (i << 12) | 7;
    }
    pd[0]=((u32)pt)|7;
    for (i = 0; i < 32; ++i) {
        pt[0x20 + i] =
            (PROCESS_MEMORY_BASE + pid * USER_SIZE + i * 4096UL) | 7;
    }
    processes[pid].cr3=(u32)pd;
}
#include "process_metadata.c"

static int spawn_process(const char name[11])
{
    unsigned pid;
    for (pid = 1;
         pid < MAX_PROCESSES && processes[pid].cr3 != 0;
         ++pid) {}
    if (pid == MAX_PROCESSES) {
        return -1;
    }
    if (load_file(
            name,
            (u8 *)(PROCESS_MEMORY_BASE + pid * USER_SIZE),
            USER_SIZE
        ) < 0) {
        return -1;
    }
    make_address_space(pid);
    processes[pid].active = 1;
    processes[pid].started = 0;
    processes[pid].esp = USER_STACK;
    processes[pid].eip = USER_BASE;
    processes[pid].ebx = (u32)USER_ARGS;
    processes[pid].esi = 0;
    processes[pid].ebp = 0;
    processes[pid].edi = 0;
    processes[pid].terminal_pid =
        current_process == 0 && processes[0].app_type == APP_GUI
            ? pid
            : processes[current_process].terminal_pid;
    processes[pid].parent_pid = current_process;
    processes[pid].waiting_child = 0;
    processes[pid].app_type = (u32)file_app_type(name);
    processes[pid].gui_window_requested = 0;
    set_process_name(pid, name);
    key_head[pid] = 0;
    key_tail[pid] = 0;
    terminal_size[pid] = 0;
    return (int)pid;
}
static int spawn_command(const char *command)
{
    char name[11];
    unsigned i = 0;
    unsigned j = 0;
    unsigned pid;
    u8 *args;

    for (i = 0; i < 11; ++i) name[i] = ' ';
    name[8] = 'B';
    name[9] = 'I';
    name[10] = 'N';
    i = 0;
    while (command[i] && command[i] != ' ') {
        char ch = command[i++];
        if (j >= 8) {
            return -1;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
        name[j++] = ch;
    }
    pid = (unsigned)spawn_process(name);
    if ((int)pid < 0) {
        return -1;
    }
    if (command[i] == ' ') {
        ++i;
    }
    args = (u8 *)(PROCESS_MEMORY_BASE + pid * USER_SIZE + 0xE000UL);
    j = 0;
    do {
        args[j] = command[i + j];
    } while (args[j++] && j < 255);
    return (int)pid;
}
static void switch_process(KernelRequest *r, unsigned next)
{
    processes[current_process].esp = r->user_esp;
    processes[current_process].eip = r->user_eip;
    processes[current_process].ebx = r->arg0;
    processes[current_process].esi = r->arg1;
    processes[current_process].ebp = r->next_ebp;
    processes[current_process].edi = r->next_edi;
    current_process = next;
    r->next_esp = processes[next].esp;
    r->next_eip = processes[next].eip;
    r->next_cr3 = processes[next].cr3;
    r->next_ebx = processes[next].ebx;
    r->next_esi = processes[next].esi;
    r->next_ebp = processes[next].ebp;
    r->next_edi = processes[next].edi;
    if (!processes[next].started) {
        processes[next].started = 1;
        r->result = (u32)USER_ARGS;
    } else {
        r->result = 0;
    }
}

static void schedule(KernelRequest *r)
{
    unsigned start = current_process;
    unsigned next = current_process;
    unsigned process;

    for (process = 1; process < MAX_PROCESSES; ++process) {
        if (processes[process].cr3 != 0 &&
            processes[process].terminal_pid == process) {
            processes[process].active = 1;
        }
    }

    do {
        next = (next + 1) % MAX_PROCESSES;
    } while ((!processes[next].active || processes[next].waiting_child) &&
             next != start);

    switch_process(r, next);
}
static int execute(const char *command)
{
    char name[11];
    unsigned i = 0;
    unsigned j = 0;
    int app_type;
    int result;

    for (i = 0; i < 11; ++i) name[i] = ' ';
    name[8] = 'B';
    name[9] = 'I';
    name[10] = 'N';
    i = 0;
    while (command[i] && command[i] != ' ') {
        char ch = command[i++];
        if (j >= 8) {
            return -1;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
        name[j++] = ch;
    }
    app_type = file_app_type(name);
    if (app_type < 0) {
        return -1;
    }
    if (app_type == APP_GUI && current_process != 0) {
        return -6;
    }
    if (command[i] == ' ') {
        ++i;
    }
    j = 0;
    do {
        USER_ARGS[j] = command[i + j];
    } while (USER_ARGS[j++] && j < 255);
    result = load_program(name);
    if (result == 0) {
        processes[current_process].app_type = (u32)app_type;
        set_process_name(current_process, name);
    }
    return result;
}

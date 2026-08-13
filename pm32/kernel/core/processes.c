static u8 *bootstrap_user_memory(void)
{
    return (u8 *)0x60000UL;
}

static u8 *user_memory(void)
{
    return (u8 *)USER_BASE;
}

static u8 *terminal_history_memory(unsigned process)
{
    return (u8 *)(PROCESS_MEMORY_BASE + process * USER_SIZE +
                  USER_IMAGE_SIZE + USER_HEAP_SIZE);
}

static void terminal_history_append(unsigned channel,
                                    const u8 *text, unsigned length)
{
    u8 *history = terminal_history_memory(channel);
    unsigned n = terminal_history_size[channel];
    unsigned i;
    for (i = 0; i < length; ++i) {
        u8 ch = text[i];
        if (ch == '\f' || ch == 0x0E) n = 0;
        else if (ch == '\b') { if (n) --n; }
        else if (ch != 0x0F) {
            if (n == TERMINAL_HISTORY_CAPACITY) {
                unsigned j;
                for (j = 1; j < n; ++j) history[j - 1] = history[j];
                --n;
            }
            history[n++] = ch;
        }
    }
    terminal_history_size[channel] = n;
}

static unsigned terminal_history_copy(unsigned process, u8 *destination)
{
    u8 *history = terminal_history_memory(process);
    unsigned n = terminal_history_size[process];
    unsigned i;
    for (i = 0; i < n; ++i) destination[i] = history[i];
    return n;
}

static void make_address_space(unsigned pid)
{
    u32 *pd=(u32 *)(PAGE_DIRECTORY_BASE+pid*4096UL);
    u32 *kernel_pt = (u32 *)(PAGE_TABLE_BASE + pid * 8192UL);
    u32 *user_pt = kernel_pt + 1024;
    unsigned i;

    for (i = 0; i < 1024; ++i) {
        /* Identity mappings are kernel-only.  User mappings are granted
           explicitly below; otherwise ring 3 could write the kernel, page
           tables and the physical memory of every other process. */
        pd[i] = (i << 22) | 0x83;
        kernel_pt[i] = (i << 12) | 3;
        user_pt[i] = 0;
    }
    /* PDE 0 contains supervisor mappings, except for the read-only BIOS font
       pointer page. PDE 1 is private user code/data; PDE 2 maps the GUI
       software backbuffer at virtual 0x800000 to physical 0x400000. */
    pd[0] = ((u32)kernel_pt) | 7;
    pd[1] = ((u32)user_pt) | 7;
    pd[2] = 0x400000UL | 0x87;
    pd[PROCESS_MEMORY_BASE >> 22] =
        PROCESS_MEMORY_PHYSICAL_BASE | 0x83;
    pd[0xFD000000UL >> 22] = 0xFD000000UL | 0x87;
    /* The GUI reads the BIOS 8x16 font pointer saved by boot at 0x500.
       Expose the pointer page and the VGA BIOS ROM window read-only; writes
       from ring 3 still fault and kernel RAM remains supervisor-only. */
    kernel_pt[0] = 5;
    for (i = 0xC0; i < 0x100; ++i)
        kernel_pt[i] = (i << 12) | 5;
    for (i = 0; i < 32; ++i) {
        user_pt[i] =
            (PROCESS_MEMORY_PHYSICAL_BASE + pid * USER_SIZE + i * 4096UL) | 7;
    }
    for (i = 0; i < 31; ++i) {
        user_pt[0x100 + i] =
            (PROCESS_MEMORY_PHYSICAL_BASE + pid * USER_SIZE +
             USER_IMAGE_SIZE + i * 4096UL) | 7;
    }
    for (i = 0; i < 64; ++i) {
        user_pt[0x1C0 + i] =
            (PROCESS_MEMORY_PHYSICAL_BASE + pid * USER_SIZE +
             0x40000UL + i * 4096UL) | 7;
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
            USER_IMAGE_SIZE
        ) < 0) {
        return -1;
    }
    make_address_space(pid);
    heap_reset(pid);
    processes[pid].active = 1;
    processes[pid].started = 0;
    processes[pid].esp = USER_STACK;
    processes[pid].eip = USER_BASE;
    processes[pid].ebx = (u32)USER_ARGS;
    processes[pid].esi = 0;
    processes[pid].ebp = 0;
    processes[pid].edi = 0;
    processes[pid].eax = (u32)USER_ARGS;
    processes[pid].ecx = 0;
    processes[pid].edx = 0;
    processes[pid].terminal_pid =
        processes[current_process].app_type == APP_GUI
            ? pid
            : processes[current_process].terminal_pid;
    processes[pid].parent_pid = current_process;
    processes[pid].waiting_child = 0;
    processes[pid].sleeping_until = 0;
    processes[pid].app_type = (u32)file_app_type(name);
    processes[pid].gui_window_requested = 0;
    processes[pid].gui_draw_head = 0;
    processes[pid].gui_draw_tail = 0;
    processes[pid].gui_event_head = 0;
    processes[pid].gui_event_tail = 0;
    set_process_name(pid, name);
    key_head[pid] = 0;
    key_tail[pid] = 0;
    terminal_size[pid] = 0;
    terminal_history_size[pid] = 0;
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
    args = (u8 *)(PROCESS_MEMORY_BASE + pid * USER_SIZE + 0x7F000UL);
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
    processes[current_process].eax = r->result;
    processes[current_process].ecx = r->saved_ecx;
    processes[current_process].edx = r->saved_edx;
    current_process = next;
    if (!processes[next].started ||
        processes[next].eip < USER_BASE ||
        processes[next].eip >= USER_BASE + USER_IMAGE_SIZE ||
        processes[next].esp < USER_BASE ||
        processes[next].esp > USER_STACK) {
        /* A process that has never run must always enter at the image base.
           Do not trust stale context fields when a dormant process is woken
           by GUI keyboard input. */
        processes[next].esp = USER_STACK;
        processes[next].eip = USER_BASE;
        processes[next].ebx = (u32)USER_ARGS;
        processes[next].esi = 0;
        processes[next].ebp = 0;
        processes[next].edi = 0;
        processes[next].started = 0;
    }
    r->next_esp = processes[next].esp;
    r->next_eip = processes[next].eip;
    r->next_cr3 = processes[next].cr3;
    r->next_ebx = processes[next].ebx;
    r->next_esi = processes[next].esi;
    r->next_ebp = processes[next].ebp;
    r->next_edi = processes[next].edi;
    r->next_ecx = processes[next].ecx;
    r->next_edx = processes[next].edx;
    if (!processes[next].started) {
        processes[next].started = 1;
        r->result = (u32)USER_ARGS;
    } else {
        r->result = processes[next].eax;
    }
}

static void schedule(KernelRequest *r)
{
    unsigned start = current_process;
    unsigned next = current_process;
    unsigned process;

    for (process = 1; process < MAX_PROCESSES; ++process) {
        if (processes[process].sleeping_until &&
            (long)(clock_millis() - processes[process].sleeping_until) >= 0)
            processes[process].sleeping_until = 0;
        if (processes[process].cr3 != 0 &&
            processes[process].terminal_pid == process) {
            processes[process].active = 1;
        }
    }

    do {
        next = (next + 1) % MAX_PROCESSES;
    } while ((!processes[next].active || processes[next].waiting_child ||
              processes[next].sleeping_until) &&
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
        heap_reset(current_process);
        processes[current_process].app_type = (u32)app_type;
        set_process_name(current_process, name);
    }
    return result;
}

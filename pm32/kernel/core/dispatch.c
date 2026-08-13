/* Single entry point called by the minimal assembly SYSENTER bridge. */
void kernel_dispatch(KernelRequest *r)
{
    r->result = 0;
    r->next_esp = r->user_esp;
    r->next_eip = r->user_eip;
    r->next_cr3 = processes[current_process].cr3;
    r->next_ebx = r->arg0;
    r->next_esi = r->arg1;

    if (r->number == 0) {
        console_clear();
        console_write("> ", 2);
    } else if (r->number == 1) {
        if (current_process == 0 ||
            processes[current_process].terminal_pid == 0) {
            console_write((const char *)r->arg0, (unsigned)r->arg1);
        }
        else {
            unsigned channel = processes[current_process].terminal_pid;
            unsigned i;
            unsigned n = terminal_size[channel];

            for (i = 0; i < r->arg1 && n < TERMINAL_CAPACITY; ++i) {
                terminal_output[channel][n++] = ((const u8 *)r->arg0)[i];
            }
            terminal_size[channel] = n;
        }
    } else if (r->number == 2) {
        if (current_process == 0) {
            /* PID 0 owns the shell or desktop and is never terminated. */
            r->result = (u32)-1;
        } else {
            unsigned process = current_process;
            unsigned parent = processes[process].parent_pid;

            processes[process].active = 0;
            processes[process].cr3 = 0;
            if (parent < MAX_PROCESSES && processes[parent].active) {
                processes[parent].waiting_child = 0;
                switch_process(r, parent);
            } else {
                schedule(r);
            }
        }
    } else if (r->number == 3) {
        r->result = (u32)execute((const char *)r->arg0);
        if ((int)r->result == 0) {
            r->result = (u32)USER_ARGS;
            r->next_esp = USER_STACK;
            r->next_eip = (u32)user_memory();
        }
    } else if (r->number == 4) {
        if (current_process == 0 ||
            processes[current_process].terminal_pid == 0) {
            r->result = (u32)read_key();
        }
        else if (key_tail[current_process] != key_head[current_process]) {
            r->result = key_queue[current_process][key_tail[current_process]];
            key_tail[current_process] =
                (key_tail[current_process] + 1) & 63;
            if (processes[current_process].app_type == APP_GUI &&
                r->result != 27) {
                unsigned channel = processes[current_process].terminal_pid;

                if (terminal_size[channel] < TERMINAL_CAPACITY) {
                    terminal_output[channel][terminal_size[channel]++] =
                        r->result == 13 ? '\n' : (u8)r->result;
                }
            }
        } else {
            r->result = 0;
        }
    } else if (r->number == 5) {
        FileRequest *f=(FileRequest *)r->arg0;
        r->result = (u32)load_file(
            f->name,
            (u8 *)f->destination,
            f->capacity
        );
    } else if (r->number == 6) {
        r->result = (u32)spawn_command((const char *)r->arg0);
        if ((int)r->result >= 0 &&
            processes[r->result].app_type != APP_CONSOLE) {
            processes[r->result].active = 0;
            processes[r->result].cr3 = 0;
            r->result = (u32)-6;
        }
    } else if (r->number == 7) {
        schedule(r);
    } else if (r->number == 8) {
        unsigned process = (unsigned)r->arg0;

        if (process < MAX_PROCESSES && processes[process].cr3 != 0) {
            unsigned next = (key_head[process] + 1) & 63;

            processes[process].active = 1;
            if (next != key_tail[process]) {
                key_queue[process][key_head[process]] = (u8)r->arg1;
                key_head[process] = next;
            }
        }
    } else if (r->number == 9) {
        unsigned process = (unsigned)r->arg0;
        unsigned i;
        u8 *to = (u8 *)r->arg1;

        if (process < MAX_PROCESSES) {
            unsigned n = terminal_size[process];

            for (i = 0; i < n; ++i) {
                to[i] = terminal_output[process][i];
            }
            terminal_size[process] = 0;
            r->result = n;
        }
    } else if (r->number == 10) {
        r->result = current_process;
    } else if (r->number == 11) {
        unsigned process = (unsigned)r->arg0;

        if (current_process == 0 && process > 0 &&
            process < MAX_PROCESSES) {
            processes[process].active = 0;
            processes[process].cr3 = 0;
            terminal_size[process] = 0;
        }
    } else if (r->number == 12) {
        unsigned i;
        u8 *source = (u8 *)USER_BASE;
        u8 *target = (u8 *)PROCESS_MEMORY_BASE;

        for (i = 0; i < USER_SIZE; ++i)
            target[i] = source[i];
        for (i = 0; i < MAX_PROCESSES; ++i) {
            processes[i].active = 0;
            processes[i].cr3 = 0;
        }
        make_address_space(0);
        processes[0].active = 1;
        processes[0].started = 1;
        processes[0].esp = USER_STACK;
        processes[0].eip = USER_BASE;
        current_process = 0;
        processes[0].terminal_pid = 0;
        processes[0].parent_pid = 0;
        processes[0].waiting_child = 0;
        processes[0].app_type = APP_CONSOLE;
        processes[0].gui_window_requested = 0;
        set_process_name(0, "SHELL   BIN");
        r->result = processes[0].cr3;
        r->next_cr3 = processes[0].cr3;
    } else if (r->number == 13) {
        r->result = (u32)load_program("SHELL   BIN");
    } else if (r->number == 14) {
        r->result = (u32)spawn_command((const char *)r->arg0);
        if ((int)r->result >= 0 &&
            processes[r->result].app_type == APP_CONSOLE) {
            processes[current_process].waiting_child = 1;
            switch_process(r, (unsigned)r->result);
        } else if ((int)r->result >= 0) {
            processes[r->result].active = 0;
            processes[r->result].cr3 = 0;
            r->result = (u32)-6;
        }
    } else if (r->number == 15) {
        WriteRequest *request = (WriteRequest *)r->arg0;

        r->result = (u32)write_file(
            request->name,
            (const u8 *)request->source,
            request->size
        );
    } else if (r->number == 16) {
        r->result = processes[current_process].app_type;
    } else if (r->number == 17) {
        r->result = (u32)spawn_command((const char *)r->arg0);
        if ((int)r->result >= 0 &&
            processes[r->result].app_type != APP_GUI) {
            processes[r->result].active = 0;
            processes[r->result].cr3 = 0;
            r->result = (u32)-6;
        }
    } else if (r->number == 18) {
        r->result = (u32)list_files((char *)r->arg0, (unsigned)r->arg1);
    } else if (r->number == 19) {
        out16(0x604, 0x2000);
        out16(0xB004, 0x2000);
        for (;;) {
            cpu_halt();
        }
    } else if (r->number == 20) {
        if (processes[current_process].app_type == APP_GUI) {
            processes[current_process].gui_window_requested = 1;
            r->result = 0;
        } else {
            r->result = (u32)-1;
        }
    } else if (r->number == 21) {
        unsigned process;

        r->result = (u32)-1;
        if (current_process == 0) {
            for (process = 1; process < MAX_PROCESSES; ++process) {
                if (processes[process].cr3 != 0 &&
                    processes[process].gui_window_requested) {
                    processes[process].gui_window_requested = 0;
                    r->result = process;
                    break;
                }
            }
        }
    } else if (r->number == 22) {
        ProcessInfo *to = (ProcessInfo *)r->arg0;
        unsigned capacity = (unsigned)r->arg1;
        unsigned count = 0;
        unsigned process;
        for (process = 0; process < MAX_PROCESSES && count < capacity; ++process) {
            if (processes[process].cr3 != 0) {
                fill_process_info(&to[count], process);
                ++count;
            }
        }
        r->result = count;
    } else if (r->number == 23) {
        const char *from = (const char *)r->arg0;
        unsigned i = 0;

        if (processes[current_process].app_type != APP_GUI) {
            r->result = (u32)-1;
        } else {
            while (i < 31 && from[i]) {
                processes[current_process].gui_title[i] = from[i];
                ++i;
            }
            processes[current_process].gui_title[i] = 0;
            r->result = i;
        }
    } else if (r->number == 24) {
        unsigned process = (unsigned)r->arg0;
        char *to = (char *)r->arg1;
        unsigned i = 0;

        if (current_process != 0 || process >= MAX_PROCESSES ||
            processes[process].cr3 == 0) {
            r->result = (u32)-1;
        } else {
            while (i < 31 && processes[process].gui_title[i]) {
                to[i] = processes[process].gui_title[i];
                ++i;
            }
            to[i] = 0;
            r->result = i;
        }
    } else {
        r->result = (u32)-1;
    }
}
#pragma aux kernel_dispatch parm [esi] modify [eax ebx ecx edx edi];

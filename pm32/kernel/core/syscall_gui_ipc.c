static int dispatch_gui_ipc_syscall(KernelRequest *r)
{
    if (r->number == 28) {
        GuiDrawCommand *from = (GuiDrawCommand *)r->arg0;
        unsigned head = processes[current_process].gui_draw_head;
        unsigned next = (head + 1) & (GUI_DRAW_QUEUE_SIZE - 1);
        if (processes[current_process].app_type != APP_GUI ||
            next == processes[current_process].gui_draw_tail)
            r->result = (u32)-1;
        else {
            processes[current_process].gui_draw_queue[head] = *from;
            processes[current_process].gui_draw_queue[head].pid = current_process;
            processes[current_process].gui_draw_queue[head].text[63] = 0;
            processes[current_process].gui_draw_head = next;
        }
        return 1;
    }
    if (r->number == 29) {
        GuiDrawCommand *to = (GuiDrawCommand *)r->arg0;
        unsigned process;
        r->result = (u32)-1;
        if (current_process == 0)
            for (process = 1; process < MAX_PROCESSES; ++process)
                if (processes[process].cr3 &&
                    processes[process].gui_draw_tail != processes[process].gui_draw_head) {
                    unsigned tail = processes[process].gui_draw_tail;
                    *to = processes[process].gui_draw_queue[tail];
                    processes[process].gui_draw_tail =
                        (tail + 1) & (GUI_DRAW_QUEUE_SIZE - 1);
                    r->result = 0;
                    break;
                }
        return 1;
    }
    if (r->number == 30) {
        GuiEvent *to = (GuiEvent *)r->arg0;
        if (processes[current_process].gui_event_tail ==
            processes[current_process].gui_event_head) r->result = 0;
        else {
            *to = processes[current_process].gui_events[
                processes[current_process].gui_event_tail];
            processes[current_process].gui_event_tail =
                (processes[current_process].gui_event_tail + 1) &
                (GUI_EVENT_QUEUE_SIZE - 1);
            r->result = 1;
        }
        return 1;
    }
    if (r->number == 31) {
        unsigned process = r->arg0;
        GuiEvent *from = (GuiEvent *)r->arg1;
        if (current_process != 0 || process >= MAX_PROCESSES ||
            !processes[process].cr3) r->result = (u32)-1;
        else {
            unsigned head = processes[process].gui_event_head;
            unsigned tail = processes[process].gui_event_tail;
            unsigned previous = (head - 1) & (GUI_EVENT_QUEUE_SIZE - 1);
            unsigned next = (head + 1) & (GUI_EVENT_QUEUE_SIZE - 1);
            if (from->type == GUI_EVENT_MOUSE && head != tail &&
                processes[process].gui_events[previous].type == GUI_EVENT_MOUSE &&
                processes[process].gui_events[previous].value == from->value) {
                processes[process].gui_events[previous] = *from;
                r->result = 0;
            } else if (next == tail) r->result = (u32)-1;
            else {
                processes[process].gui_events[head] = *from;
                processes[process].gui_event_head = next;
                r->result = 0;
            }
        }
        return 1;
    }
    if (r->number == 32) {
        unsigned process = r->arg0;
        if (process >= MAX_PROCESSES || !processes[process].cr3 ||
            (current_process != 0 &&
             processes[current_process].app_type != APP_GUI)) r->result = (u32)-1;
        else r->result = terminal_history_copy(process, (u8 *)r->arg1);
        return 1;
    }
    if (r->number == 33) {
        unsigned process = r->arg0;
        if (process == 0 ||
            (process < MAX_PROCESSES && processes[process].cr3)) {
            gui_focus_pid = process;
            r->result = 0;
        } else r->result = (u32)-1;
        return 1;
    }
    if (r->number == 34) {
        r->result = gui_focus_pid;
        return 1;
    }
    return 0;
}

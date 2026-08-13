static int dispatch_time_syscall(KernelRequest *r)
{
    if (r->number == 7) {
        cpu_wait_interrupt();
        schedule(r);
        return 1;
    }
    if (r->number == 27) {
        r->result = clock_millis();
        return 1;
    }
    if (r->number == 36) {
        u32 deadline = r->arg0;
        if ((long)(clock_millis() - deadline) < 0) {
            unsigned sleeper = current_process;
            processes[current_process].sleeping_until = deadline;
            schedule(r);
            if (current_process == sleeper) {
                while ((long)(clock_millis() - deadline) < 0)
                    cpu_wait_interrupt();
                processes[sleeper].sleeping_until = 0;
            }
        }
        return 1;
    }
    return 0;
}

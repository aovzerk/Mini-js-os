static int dispatch_memory_syscall(KernelRequest *r)
{
    if (r->number == 25) {
        r->result = (u32)heap_allocate(r->arg0);
        return 1;
    }
    if (r->number == 26) {
        r->result = (u32)heap_release((void *)r->arg0);
        return 1;
    }
    return 0;
}

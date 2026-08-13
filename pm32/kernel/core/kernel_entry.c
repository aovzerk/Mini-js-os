/* Stable flat-binary C entry. Assembly calls this first symbol only. */
u32 kernel_dispatch(KernelRequest *r)
{
    if (r->number == KERNEL_BOOT_INIT) {
        console_init();
        return 0;
    }
    return kernel_dispatch_impl(r);
}
#pragma aux kernel_dispatch parm [esi] value [eax] modify [eax ebx ecx edx edi];

; Minimal SYSENTER/SYSEXIT architecture bridge. Policy lives in core.c.
sysenter_entry:
    cli
    push edi
    mov edi, eax
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax
    mov dword [kernel_request+0], edi
    mov dword [kernel_request+4], ebx
    mov dword [kernel_request+8], esi
    mov dword [kernel_request+12], ecx
    mov dword [kernel_request+16], edx
    mov dword [kernel_request+44], ebp
    mov ebp, dword [esp]
    mov dword [kernel_request+48], ebp

    push ebx
    push esi
    push ebp
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax
    add esp, 16

    mov eax, dword [kernel_request+20]
    mov ecx, dword [kernel_request+24]
    mov edx, dword [kernel_request+28]
    mov esi, dword [kernel_request+32]
    mov cr3, esi
    mov ebx, dword [kernel_request+36]
    mov esi, dword [kernel_request+40]
    mov ebp, dword [kernel_request+44]
    mov edi, dword [kernel_request+48]
    push eax
    mov ax, SEL_UDATA or 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    pop eax
    sysexit

halt_kernel:
    cli
    hlt
    jmp halt_kernel

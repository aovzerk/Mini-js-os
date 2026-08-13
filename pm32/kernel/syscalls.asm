; Ring-3 syscall gate. INT 80h supplies a real hardware return frame, so a
; process switch can replace CR3/EIP/ESP atomically and return with IRETD.
int80_entry:
    cli
    push eax
    push edi
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax

    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    ; One supervisor-only trap frame per address space. CR3 encodes PID in
    ; 4 KiB steps starting at PAGE_DIRECTORY_BASE (0x80000).
    mov eax, cr3
    sub eax, 0x80000
    shr eax, 12
    imul eax, 0x48
    add eax, SYSTEM_PHYSICAL + process_requests
    mov edi, eax
    mov eax, dword [esp + 0x04]
    mov dword [edi + 0x00], eax
    mov dword [edi + 0x04], ebx
    mov dword [edi + 0x08], esi
    mov eax, dword [esp + 0x14]
    mov dword [edi + 0x0C], eax
    mov eax, dword [esp + 0x08]
    mov dword [edi + 0x10], eax
    mov dword [edi + 0x2C], ebp
    mov eax, dword [esp]
    mov dword [edi + 0x30], eax
    mov eax, dword [esp + 0x04]
    mov dword [edi + 0x34], eax
    mov dword [edi + 0x38], ecx
    mov dword [edi + 0x3C], edx
    mov esi, edi
    push esi
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    pop esi

    mov edx, dword [esi + 0x20]
    mov cr3, edx

    ; Discard the old interrupt frame and construct the selected process's
    ; ring-3 frame on the dedicated kernel stack.
    mov esp, KERNEL_STACK_LINEAR
    ; Return the value recorded in the request block.  EAX is also used by
    ; the dispatcher and CR3 switching path, so treating its incidental
    ; value as the syscall result leaked the page-directory address to user
    ; code (for example getPid() returned 0x80000).
    mov eax, dword [esi + 0x14]
    push eax
    push SEL_UDATA or 3
    push dword [esi + 0x18]
    push 0x3202
    push SEL_UCODE or 3
    push dword [esi + 0x1C]

    mov ebx, dword [esi + 0x24]
    mov ebp, dword [esi + 0x2C]
    mov edi, dword [esi + 0x30]
    mov ecx, dword [esi + 0x40]
    mov edx, dword [esi + 0x44]
    mov esi, dword [esi + 0x28]
    ; Ring 3 must receive usable data selectors. Leaving SEL_KDATA loaded
    ; caused the first access to the shell command buffer to fault.
    mov dx, SEL_UDATA or 3
    mov ds, dx
    mov es, dx
    mov eax, dword [esp + 0x14]
    iretd

exception_entry:
    cli
.halt:
    hlt
    jmp .halt

irq0_entry:
    pushad
    push ds
    push es
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    inc dword [0x70004]
    inc byte [0x70008]
    cmp byte [0x70008], 10
    jb .return
    mov byte [0x70008], 0
    test byte [esp + 0x2C], 3
    jz .return
    mov al, 0x20
    out 0x20, al

    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x00], 35
    mov eax, dword [esp + 0x18]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x04], eax
    mov eax, dword [esp + 0x0C]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x08], eax
    mov eax, dword [esp + 0x34]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x0C], eax
    mov eax, dword [esp + 0x28]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x10], eax
    mov eax, dword [esp + 0x10]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x2C], eax
    mov eax, dword [esp + 0x08]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x30], eax
    mov eax, dword [esp + 0x24]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x34], eax
    mov eax, dword [esp + 0x20]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x38], eax
    mov eax, dword [esp + 0x1C]
    mov dword [SYSTEM_PHYSICAL + kernel_request + 0x3C], eax

    mov esi, SYSTEM_PHYSICAL + kernel_request
    push esi
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    pop esi
    mov eax, dword [esi + 0x20]
    mov cr3, eax
    mov esp, KERNEL_STACK_LINEAR
    push dword [esi + 0x14]
    push SEL_UDATA or 3
    push dword [esi + 0x18]
    push 0x3202
    push SEL_UCODE or 3
    push dword [esi + 0x1C]
    mov ebx, dword [esi + 0x24]
    mov ebp, dword [esi + 0x2C]
    mov edi, dword [esi + 0x30]
    mov ecx, dword [esi + 0x40]
    mov edx, dword [esi + 0x44]
    mov esi, dword [esi + 0x28]
    mov ax, SEL_UDATA or 3
    mov ds, ax
    mov es, ax
    mov eax, dword [esp + 0x14]
    iretd
.return:
    mov al, 0x20
    out 0x20, al
    pop es
    pop ds
    popad
    iretd

irq1_entry:
    push ds
    push eax
    push ebx
    mov ax, SEL_FLAT
    mov ds, ax
    in al, 0x60
    movzx ebx, byte [0x70000]
    mov byte [0x70010 + ebx], al
    inc bl
    and bl, 0x3F
    mov byte [0x70000], bl
    mov al, 0x20
    out 0x20, al
    pop ebx
    pop eax
    pop ds
    iretd

irq12_entry:
    push ds
    push eax
    push ebx
    push ecx
    push edx
    mov ax, SEL_FLAT
    mov ds, ax
    mov edx, 16
.mouse_drain:
    in al, 0x64
    test al, 1
    jz .mouse_done
    ; IRQ12 must not steal a keyboard byte which happens to be waiting in the
    ; shared controller output buffer.
    test al, 0x20
    jz .mouse_done
    mov cl, al
    in al, 0x60
    test cl, 0xC0
    jz .mouse_status_ok
    mov byte [0x70009], 0
    jmp .mouse_continue
.mouse_status_ok:
    mov cl, byte [0x70009]
    test cl, cl
    jnz .mouse_next
    test al, 8
    jz .mouse_continue
    mov byte [0x7000C], al
    mov byte [0x70009], 1
    jmp .mouse_continue
.mouse_next:
    cmp cl, 1
    jne .mouse_last
    mov byte [0x7000D], al
    mov byte [0x70009], 2
    jmp .mouse_continue
.mouse_last:
    mov byte [0x7000E], al
    mov byte [0x70009], 0
    movzx ebx, byte [0x70002]
    mov cl, bl
    inc cl
    and cl, 0x1F
    cmp cl, byte [0x70003]
    je .mouse_full
    mov eax, dword [0x7000C]
    and eax, 0x00FFFFFF
    or eax, 0x01000000
    mov dword [0x70080 + ebx * 4], eax
    mov byte [0x70002], cl
.mouse_full:
.mouse_continue:
    dec edx
    jnz .mouse_drain
.mouse_done:
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    pop edx
    pop ecx
    pop ebx
    pop eax
    pop ds
    iretd


halt_kernel:
    cli
    hlt
    jmp halt_kernel

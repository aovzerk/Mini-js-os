format binary
use16
org 0
SYSTEM_PHYSICAL = 0x8000
USER_PHYSICAL   = 0x60000
USER_VIRTUAL    = 0x400000
DISK_CACHE      = 0x40000
KERNEL_C_PHYSICAL = 0x20000
; Keep the ring-0 stack in dedicated low memory, away from the kernel image,
; disk cache, bootstrap user image, IRQ queue and page tables.
KERNEL_STACK_OFFSET = 0x57000
KERNEL_STACK_LINEAR = SYSTEM_PHYSICAL + KERNEL_STACK_OFFSET
USER_STACK_LINEAR   = 0x600000

SEL_KCODE = 0x08
SEL_KDATA = 0x10
SEL_UCODE = 0x18
SEL_UDATA = 0x20
SEL_TSS   = 0x28
SEL_VIDEO = 0x30
SEL_FLAT  = 0x38

start16:
    cli
    cld
    mov ax, cs
    mov ds, ax
    lgdt [gdtr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp SEL_KCODE:protected_start

use32
protected_start:
    cld
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax
    mov ax, SEL_FLAT
    mov ss, ax
    mov esp, KERNEL_STACK_LINEAR
    mov ax, SEL_KDATA
    mov ax, SEL_VIDEO
    mov gs, ax

    ; Remap the legacy PIC away from CPU exception vectors.
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    ; Enable timer, keyboard and the cascade carrying mouse IRQ12.
    mov al, 0xF8
    out 0x21, al
    mov al, 0xEF
    out 0xA1, al

    ; DS is based at SYSTEM_PHYSICAL here; these offsets address physical
    ; 0x70000, shared with the flat C kernel and IRQ handler.
    mov byte [0x68000], 0
    mov byte [0x68001], 0
    mov byte [0x68002], 0
    mov byte [0x68003], 0
    mov byte [0x68009], 0
    mov dword [0x6800C], 0
    mov dword [0x68004], 0
    mov byte [0x68008], 0

    ; 1000 Hz scheduler/timer tick.  The BIOS default (~18.2 Hz) made every
    ; sys_yield cost up to 55 ms, accumulating across GUI draw commands.
    mov al, 0x36
    out 0x43, al
    mov ax, 0x04A9
    out 0x40, al
    mov al, ah
    out 0x40, al
    ; Enable the first PS/2 port interrupt in the controller command byte.
.ps2_wait_input_1:
    in al, 0x64
    test al, 2
    jnz .ps2_wait_input_1
    mov al, 0x20
    out 0x64, al
.ps2_wait_output:
    in al, 0x64
    test al, 1
    jz .ps2_wait_output
    in al, 0x60
    or al, 3
    and al, 0xEF
    mov ah, al
.ps2_wait_input_2:
    in al, 0x64
    test al, 2
    jnz .ps2_wait_input_2
    mov al, 0x60
    out 0x64, al
.ps2_wait_input_3:
    in al, 0x64
    test al, 2
    jnz .ps2_wait_input_3
    mov al, ah
    out 0x60, al
    ; Bring up the auxiliary PS/2 device before IRQ delivery starts.
    mov ecx, 100000
.mouse_wait_a8:
    in al, 0x64
    test al, 2
    loopnz .mouse_wait_a8
    mov al, 0xA8
    out 0x64, al
    mov bl, 0xF6
    call ps2_mouse_boot_command
    mov bl, 0xF4
    call ps2_mouse_boot_command
    ; Some controllers expose an extra ACK/response after the command helper
    ; has consumed one byte. Drain all pending boot responses so IRQ1/IRQ12
    ; start exactly at a scan-code or at byte zero of a mouse packet.
    mov ecx, 100000
.ps2_drain:
    in al, 0x64
    test al, 1
    jz .ps2_drained
    in al, 0x60
    loop .ps2_drain
.ps2_drained:
    mov byte [0x68000], 0
    mov byte [0x68001], 0
    mov byte [0x68002], 0
    mov byte [0x68003], 0
    mov byte [0x68009], 0
    mov dword [0x6800C], 0

    lidt [idtr]

    mov dword [tss + 4], KERNEL_STACK_LINEAR
    mov word [tss + 8], SEL_FLAT
    mov ax, SEL_TSS
    ltr ax

    ; Initialize the C console through the same request ABI as syscalls.
    mov dword [kernel_request], 0xFFFFFFFF
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax

    ; Load the initial shell from FAT32 into the bootstrap user area.
    mov dword [kernel_request], 0x0D
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax

    ; Ask C to build the first process page directory and copy its image.
    mov dword [kernel_request], 0x0C
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    ; Bootstrap process zero: enforce its ring-3 page-directory entry before
    ; paging is enabled. Later processes receive it from make_address_space().
    mov dword [0x80000], 0x90000 or 0x07
    mov dword [0x80000 + 0x04], 0x91000 or 0x07
    mov dword [0x91000], 0x800000 or 0x07
    ; Process zero always owns the first page directory.
    mov eax, 0x80000
    mov cr3, eax
    mov eax, cr4
    or eax, 1 shl 4
    mov cr4, eax
    mov eax, cr0
    ; Paging plus supervisor write protection.  Ring 3 cannot access pages
    ; without U/S, and ring 0 must also respect read-only PTEs.
    or eax, (1 shl 0x1F) or (1 shl 0x10)
    mov cr0, eax
    jmp $+2
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax


    ; Enter CPL3 using an IRET frame.
    push SEL_UDATA or 3
    push USER_STACK_LINEAR
    pushfd
    or dword [esp], 0x3200      ; IOPL3 plus hardware interrupts enabled
    push SEL_UCODE or 3
    push USER_VIRTUAL
    xor eax, eax
    iretd

ps2_mouse_boot_command:
    mov ecx, 100000
.wait_command:
    in al, 0x64
    test al, 2
    loopnz .wait_command
    mov al, 0xD4
    out 0x64, al
    mov ecx, 100000
.wait_data:
    in al, 0x64
    test al, 2
    loopnz .wait_data
    mov al, bl
    out 0x60, al
    mov ecx, 100000
.wait_ack:
    in al, 0x64
    test al, 1
    jnz .consume
    loop .wait_ack
    ret
.consume:
    in al, 0x60
    ret


include 'pm32/kernel/syscalls.asm'
include 'pm32/kernel/x86_tables.asm'

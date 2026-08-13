format binary
use16
org 0

SYSTEM_PHYSICAL = 0x8000
USER_PHYSICAL   = 0x20000
DISK_CACHE      = 0x40000
KERNEL_C_PHYSICAL = 0x10000

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
    mov ss, ax
    mov esp, 0xF000
    mov ax, SEL_VIDEO
    mov gs, ax

    mov dword [tss + 4], 0xF000
    mov word [tss + 8], SEL_KDATA
    mov ax, SEL_TSS
    ltr ax

    mov eax, 1
    cpuid
    test edx, 1 shl 11          ; SEP: SYSENTER/SYSEXIT available
    jz no_sysenter

    ; IA32_SYSENTER_CS, IA32_SYSENTER_ESP, IA32_SYSENTER_EIP.
    mov ecx, 0x174
    mov eax, SEL_KCODE
    xor edx, edx
    wrmsr
    mov ecx, 0x175
    mov eax, SYSTEM_PHYSICAL + 0xF000
    wrmsr
    mov ecx, 0x176
    mov eax, SYSTEM_PHYSICAL + sysenter_entry
    wrmsr

    ; Initialize the C console through the same request ABI as syscalls.
    mov dword [kernel_request], 0
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax

    ; Load the initial shell from FAT32 into the bootstrap user area.
    mov dword [kernel_request], 13
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax

    ; Ask C to build the first process page directory and copy its image.
    mov dword [kernel_request], 12
    mov esi, SYSTEM_PHYSICAL + kernel_request
    mov ax, SEL_FLAT
    mov ds, ax
    mov es, ax
    call KERNEL_C_PHYSICAL - SYSTEM_PHYSICAL
    ; Process zero always owns the first page directory.
    mov eax, 0x80000
    mov cr3, eax
    mov eax, cr4
    or eax, 1 shl 4
    mov cr4, eax
    mov eax, cr0
    or eax, 1 shl 31
    mov cr0, eax
    jmp $+2
    mov ax, SEL_KDATA
    mov ds, ax
    mov es, ax


    ; Enter CPL3 using an IRET frame.
    push SEL_UDATA or 3
    push USER_PHYSICAL + 0xF000
    pushfd
    or dword [esp], 0x3000      ; IOPL3 for current VGA/PS2 user drivers
    push SEL_UCODE or 3
    push USER_PHYSICAL
    mov eax, 1                  ; shell knows the first prompt is already visible
    iretd

no_sysenter:
    cli
    hlt

include 'pm32/kernel/syscalls.asm'
include 'pm32/kernel/x86_tables.asm'

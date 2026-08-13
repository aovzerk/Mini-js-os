align 4
kernel_request rb 0x48
process_requests rb 8 * 0x48

align 0x10
tss rb 0x68
tss_end:

align 8
gdt:
    dq 0
    ; Kernel code/data use the address where the assembly entry is loaded.
    dw 0xFFFF, SYSTEM_PHYSICAL and 0xFFFF
    db (SYSTEM_PHYSICAL shr 16) and 0xFF, 0x9A, 0xCF
    db (SYSTEM_PHYSICAL shr 24) and 0xFF
    dw 0xFFFF, SYSTEM_PHYSICAL and 0xFFFF
    db (SYSTEM_PHYSICAL shr 16) and 0xFF, 0x92, 0xCF
    db (SYSTEM_PHYSICAL shr 24) and 0xFF
    ; Flat ring-3 code/data required by SYSEXIT.
    dw 0xFFFF, 0x0000
    db 0x00, 0xFA, 0xCF, 0x00
    dw 0xFFFF, 0x0000
    db 0x00, 0xF2, 0xCF, 0x00
    ; 32-bit available TSS at physical SYSTEM_PHYSICAL + tss.
    dw tss_end - tss - 1
    dw (SYSTEM_PHYSICAL + tss) and 0xFFFF
    db ((SYSTEM_PHYSICAL + tss) shr 16) and 0xFF
    db 0x89
    db 0x00
    db ((SYSTEM_PHYSICAL + tss) shr 24) and 0xFF
    ; VGA text memory.
    dw 0xFFFF, 0x8000
    db 0x0B, 0x92, 0x40, 0x00
    ; Flat kernel data for physical ATA destinations.
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdtr:
    dw gdt_end - gdt - 1
    dd SYSTEM_PHYSICAL + gdt

macro interrupt_gate target, attributes
{
    dw target and 0xFFFF
    dw SEL_KCODE
    db 0
    db attributes
    dw (target shr 16) and 0xFFFF
}

align 8
idt:
repeat 0x100
    if % <= 0x20
        interrupt_gate exception_entry, 0x8E
    else if % = 0x21
        interrupt_gate irq0_entry, 0x8E
    else if % = 0x22
        interrupt_gate irq1_entry, 0x8E
    else if % = 0x2D
        interrupt_gate irq12_entry, 0x8E
    else if % = 0x81
        interrupt_gate int80_entry, 0xEE
    else
        dq 0
    end if
end repeat
idt_end:

idtr:
    dw idt_end - idt - 1
    dd SYSTEM_PHYSICAL + idt


times 0x2000 - ($ - $$) db 0
file '../../build32/kernel-core.bin'

align 4
kernel_request rb 52

align 16
tss rb 104
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


times 0x2000 - ($ - $$) db 0
file '../../build32/kernel-core.bin'

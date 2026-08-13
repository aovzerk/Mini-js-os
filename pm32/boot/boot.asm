format binary
use16
org 0x7C00

SYSTEM_SEGMENT = 0x0800
SYSTEM_ENTRY_SECTORS = 0x10
KERNEL_SEGMENT = 0x2000
if ~ defined KERNEL_SECTORS
    KERNEL_SECTORS = 0x60
end if

jmp short start
nop
db 'MYOS32  '
dw 0x0200
db 1
dw 0x0080
db 2
dw 0, 0
db 0xF8
dw 0
dw 0x003F, 0x00FF
dd 0
dd 0x00020000
dd 0x000003F1
dw 0, 0
dd 2
dw 0xFFFF, 0xFFFF
times 0x0C db 0
db 0x80, 0, 0x29
dd 0x32334F53
db 'MYOS32     '
db 'FAT32   '

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    ; Save the BIOS 8x16 font linear address for the protected-mode GUI.
    mov ax, 0x1130
    mov bh, 0x06
    int 0x10
    xor eax, eax
    mov ax, es
    shl eax, 4
    xor edx, edx
    mov dx, bp
    add eax, edx
    mov [0x0500], eax
    xor ax, ax
    mov ds, ax
    mov es, ax

    mov word [dap.count], SYSTEM_ENTRY_SECTORS
    mov word [dap.offset], 0
    mov word [dap.segment], SYSTEM_SEGMENT
    mov dword [dap.lba_low], 1
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error

    ; The C kernel is linked at 0x20000 and starts at file offset 0x2000.
    mov word [dap.count], 1
    mov word [dap.offset], 0
    mov word [dap.segment], KERNEL_SEGMENT
    mov dword [dap.lba_low], 1 + SYSTEM_ENTRY_SECTORS
    mov word [kernel_remaining], KERNEL_SECTORS
.load_kernel_sector:
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error
    add word [dap.segment], 0x20
    inc dword [dap.lba_low]
    dec word [kernel_remaining]
    jnz .load_kernel_sector

    ; Cache the FAT32 root and the following contiguous program clusters.
    mov word [dap.count], 0x007F
    mov word [dap.offset], 0
    mov word [dap.segment], 0x4000
    mov dword [dap.lba_low], 0x80 + 2*0x3F1
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error
    mov word [dap.count], 0x007F
    mov word [dap.segment], 0x4FE0
    mov dword [dap.lba_low], 0x80 + 2*0x3F1 + 0x7F
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error
    mov word [dap.count], 0x007F
    mov word [dap.segment], 0x5FC0
    mov dword [dap.lba_low], 0x80 + 2*0x3F1 + 0xFE
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error
    mov word [dap.count], 0x007F
    mov word [dap.segment], 0x6FA0
    mov dword [dap.lba_low], 0x80 + 2*0x3F1 + 0x17D
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc error
    jmp SYSTEM_SEGMENT:0

error:
    mov si, message
.print:
    lodsb
    test al, al
    jz $
    mov ah, 0x0E
    int 0x10
    jmp .print

drive db 0
kernel_remaining dw 0
message db 'PM32 load error', 0
align 4
dap:
    db 0x10, 0
.count dw 0
.offset dw 0
.segment dw 0
.lba_low dd 0
.lba_high dd 0

times 0x1FE - ($ - $$) db 0
dw 0xAA55

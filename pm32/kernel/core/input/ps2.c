#define PS2_KEY_HEAD    ((volatile u8 *)0x70000UL)
#define PS2_KEY_TAIL    ((volatile u8 *)0x70001UL)
#define PS2_MOUSE_HEAD  ((volatile u8 *)0x70002UL)
#define PS2_MOUSE_TAIL  ((volatile u8 *)0x70003UL)
#define PS2_KEY_QUEUE   ((volatile u8 *)0x70010UL)
#define PS2_MOUSE_QUEUE ((volatile u32 *)0x70080UL)

static int ps2_read_keyboard_byte(u8 *value)
{
    if (*PS2_KEY_TAIL == *PS2_KEY_HEAD) return 0;
    *value = PS2_KEY_QUEUE[*PS2_KEY_TAIL];
    *PS2_KEY_TAIL = (u8)((*PS2_KEY_TAIL + 1) & 63);
    return 1;
}

static int ps2_read_mouse_packet(u32 *packet)
{
    if (*PS2_MOUSE_TAIL == *PS2_MOUSE_HEAD) return 0;
    *packet = PS2_MOUSE_QUEUE[*PS2_MOUSE_TAIL];
    *PS2_MOUSE_TAIL = (u8)((*PS2_MOUSE_TAIL + 1) & 31);
    return 1;
}

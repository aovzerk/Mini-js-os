static int mouse_read_packet(u32 *packet)
{
    u32 current;
    int dx = 0;
    int dy = 0;
    u8 buttons = 0;
    int found = 0;

    /* Collapse a burst into one complete packet. This bounds input latency
       without ever discarding one byte from the middle of a PS/2 packet. */
    while (ps2_read_mouse_packet(&current)) {
        dx += (signed char)(current >> 8);
        dy += (signed char)(current >> 16);
        buttons = (u8)current & 7;
        found = 1;
    }
    if (!found) return 0;
    if (dx < -127) dx = -127;
    if (dx > 127) dx = 127;
    if (dy < -127) dy = -127;
    if (dy > 127) dy = 127;
    *packet = 0x01000000UL | 8 | buttons |
              (dx < 0 ? 0x10UL : 0) |
              (dy < 0 ? 0x20UL : 0) |
              ((u32)(u8)dx << 8) | ((u32)(u8)dy << 16);
    return 1;
}

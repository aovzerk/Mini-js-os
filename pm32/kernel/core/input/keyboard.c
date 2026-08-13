static int read_key(void)
{
    u8 code;
    while (ps2_read_keyboard_byte(&code)) {
        if (code == 0xE0) {
            keyboard_extended = 1;
            continue;
        }
        if (keyboard_extended) {
            keyboard_extended = 0;
            if (!(code & 0x80) && code == 0x49) console_page_up();
            else if (!(code & 0x80) && code == 0x51) console_page_down();
            continue;
        }
        if (code == 0x2A || code == 0x36) {
            keyboard_shift = 1;
            continue;
        }
        if (code == 0xAA || code == 0xB6) {
            keyboard_shift = 0;
            continue;
        }
        if (code & 0x80) continue;
        if (code == 0x39) return ' ';
        code = keyboard_shift ? scan_ascii_shift[code] : scan_ascii[code];
        if (code) return code;
    }
    return 0;
}

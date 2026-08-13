static void update_cursor(void)
{
    unsigned cell=cursor;
    out8(0x3D4, 0x0F);
    out8(0x3D5, (u8)cell);
    out8(0x3D4, 0x0E);
    out8(0x3D5, (u8)(cell >> 8));
}
static void console_clear(void)
{
    unsigned i;
    for (i = 0; i < 80 * 25; ++i)
        VGA[i] = 0x0720;
    cursor = 0;
    update_cursor();
}
static void console_write(const char *text, unsigned length)
{
    unsigned i;
    for (i = 0; i < length; ++i) {
        u8 ch = (u8)text[i];

        if (ch == '\n') {
            cursor = ((cursor / 80) + 1) * 80;
        }
        else if (ch == '\b') {
            if (cursor) {
                --cursor;
                VGA[cursor] = 0x0F20;
            }
        }
        else {
            VGA[cursor++] = (u16)(0x0F00 | ch);
        }
        if (cursor >= 80 * 25) {
            cursor = 0;
        }
    }
    update_cursor();
}
static int read_key(void)
{
    u8 code;
    if (!(in8(0x64) & 1)) {
        return 0;
    }
    code = in8(0x60);
    if (code == 0x2A || code == 0x36) {
        keyboard_shift = 1;
        return 0;
    }
    if (code == 0xAA || code == 0xB6) {
        keyboard_shift = 0;
        return 0;
    }
    if (code & 0x80) {
        return 0;
    }
    if (code == 0x39) {
        return ' ';
    }
    return keyboard_shift ? scan_ascii_shift[code] : scan_ascii[code];
}

static void update_cursor(void)
{
    unsigned cell = cursor;
    out8(0x3D4, 0x0F);
    out8(0x3D5, (u8)cell);
    out8(0x3D4, 0x0E);
    out8(0x3D5, (u8)(cell >> 8));
}

static unsigned console_bottom_start(void)
{
    return console_row >= 24 ? console_row - 24 : 0;
}

static void console_render(void)
{
    unsigned bottom = console_bottom_start();
    unsigned start = bottom - console_view_offset;
    unsigned screen_row;
    unsigned column;

    for (screen_row = 0; screen_row < 25; ++screen_row) {
        unsigned history_row = start + screen_row;
        for (column = 0; column < 80; ++column) {
            VGA[screen_row * 80 + column] = history_row <= console_row
                ? console_history[history_row * 80 + column]
                : 0x0720;
        }
    }
    if (!console_view_offset)
        cursor = (console_row - start) * 80 + console_column;
    else
        cursor = 80 * 25 - 1;
    update_cursor();
}

static void console_clear(void)
{
    unsigned i;
    for (i = 0; i < CONSOLE_HISTORY_ROWS * 80; ++i)
        console_history[i] = 0x0720;
    console_row = 0;
    console_column = 0;
    console_view_offset = 0;
    console_render();
}

static void console_advance_line(void)
{
    unsigned i;
    ++console_row;
    console_column = 0;
    if (console_row < CONSOLE_HISTORY_ROWS) return;
    for (i = 0; i < (CONSOLE_HISTORY_ROWS - 1) * 80; ++i)
        console_history[i] = console_history[i + 80];
    for (i = (CONSOLE_HISTORY_ROWS - 1) * 80;
         i < CONSOLE_HISTORY_ROWS * 80; ++i)
        console_history[i] = 0x0720;
    console_row = CONSOLE_HISTORY_ROWS - 1;
}

static void console_page_up(void)
{
    unsigned available = console_bottom_start();
    unsigned step = available - console_view_offset;
    if (step > 20) step = 20;
    console_view_offset += step;
    console_render();
}

static void console_page_down(void)
{
    if (console_view_offset > 20) console_view_offset -= 20;
    else console_view_offset = 0;
    console_render();
}

static void console_write(const char *text, unsigned length)
{
    unsigned i;
    console_view_offset = 0;
    for (i = 0; i < length; ++i) {
        u8 ch = (u8)text[i];

        if (ch == '\f') {
            console_clear();
        } else if (ch == '\n') {
            console_advance_line();
        } else if (ch == '\r') {
            console_column = 0;
        } else if (ch == '\b') {
            if (console_column) {
                --console_column;
                console_history[console_row * 80 + console_column] = 0x0720;
            }
        } else {
            console_history[console_row * 80 + console_column] =
                (u16)(0x0F00 | ch);
            ++console_column;
            if (console_column >= 80) console_advance_line();
        }
    }
    console_render();
}

static int read_key(void)
{
    u8 code;
    if (!(in8(0x64) & 1)) return 0;
    code = in8(0x60);
    if (code == 0xE0) {
        keyboard_extended = 1;
        return 0;
    }
    if (keyboard_extended) {
        keyboard_extended = 0;
        if (!(code & 0x80) && code == 0x49) console_page_up();
        else if (!(code & 0x80) && code == 0x51) console_page_down();
        return 0;
    }
    if (code == 0x2A || code == 0x36) {
        keyboard_shift = 1;
        return 0;
    }
    if (code == 0xAA || code == 0xB6) {
        keyboard_shift = 0;
        return 0;
    }
    if (code & 0x80) return 0;
    if (code == 0x39) return ' ';
    return keyboard_shift ? scan_ascii_shift[code] : scan_ascii[code];
}

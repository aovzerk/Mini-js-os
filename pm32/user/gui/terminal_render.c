static void draw_char(int x, int y, u8 ch)
{
    const u8 *font = (const u8 *)(*(volatile u32 *)0x500);
    int yy;
    int xx;

    for (yy = 0; yy < 16; ++yy) {
        u8 row = font[(unsigned)ch * 16 + yy];
        for (xx = 0; xx < 8; ++xx)
            if (row & (0x80 >> xx))
                fill(x + xx, y + yy, 1, 1, 0xE8E8E8);
    }
}

static void draw_text(int x, int y, const char *text)
{
    while (*text) {
        draw_char(x, y, (u8)*text++);
        x += 8;
    }
}

static void draw_terminal(TerminalWindow *window)
{
    int i;
    int column = 0;
    int line_number = 0;
    int x;
    int y;

    for (i = 0; i < window->terminal_size; ++i) {
        u8 ch = window->terminal[i];

        if (ch == '\n') {
            column = 0;
            ++line_number;
        } else if (ch == '\b') {
            if (column > 0) {
                --column;
            }
        } else {
            if (line_number >= window->scroll_line &&
                line_number < window->scroll_line +
                    TERMINAL_VISIBLE_LINES) {
                x = window->x + 26 + column * 8;
                y = window->y + 61 +
                    (line_number - window->scroll_line) * 18;
                draw_char(x, y, ch);
            }
            ++column;
            if (column >= TERMINAL_COLUMNS) {
                column = 0;
                ++line_number;
            }
        }
    }
    if (caret_visible && window == &windows[active_window] &&
        line_number >= window->scroll_line &&
        line_number < window->scroll_line + TERMINAL_VISIBLE_LINES) {
        x = window->x + 26 + column * 8;
        y = window->y + 61 +
            (line_number - window->scroll_line) * 18;
        fill(x, y + 14, 7, 2, 0xE8E8E8);
    }
}

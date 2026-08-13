static void redraw(void)
{
    int window;
    int y;

    fb = backbuffer;
    fill(0, 0, WIDTH, HEIGHT, 0x071019);
    for (y = 0; y < HEIGHT - 52; y += 32) {
        fill(0, y, WIDTH, 1, 0x0B1A26);
    }
    fill(0, 0, 5, HEIGHT - 52, 0x15394A);
    fill(0, HEIGHT - 52, WIDTH, 52, 0x091722);
    fill(0, HEIGHT - 52, WIDTH, 1, 0x1B4054);
    fill(WIDTH - 220, HEIGHT - 38, 150, 24, 0x102B3B);
    draw_text(WIDTH - 202, HEIGHT - 34, "MYOS 32  ONLINE");

    fill(22, 22, 76, 92, 0x0B1D29);
    fill(22, 22, 76, 2, 0x20D7E5);
    fill(28, 28, 64, 64, 0x102B3B);
    if (icon_width) {
        draw_icon(36, 36);
    }
    draw_text(28, 96, "TERMINAL");
    fill(110, 22, 76, 92, 0x0B1D29);
    fill(110, 22, 76, 2, 0xA66CFF);
    fill(116, 28, 64, 64, 0x102B3B);
    fill(130, 38, 36, 44, 0xDCE8EF);
    fill(136, 47, 24, 2, 0x466273);
    fill(136, 56, 24, 2, 0x466273);
    fill(136, 65, 18, 2, 0x466273);
    draw_text(124, 96, "EDITOR");
    fill(198, 22, 76, 92, 0x0B1D29);
    fill(198, 22, 76, 2, 0x55D88A);
    fill(204, 28, 64, 64, 0x102B3B);
    fill(214, 40, 44, 32, 0x163746);
    line(218, 64, 226, 51, 0x55D88A);
    line(226, 51, 236, 59, 0x55D88A);
    line(236, 59, 253, 43, 0x55D88A);
    draw_text(204, 96, "MONITOR");
    fill(12, HEIGHT - 43, 34, 32, 0x2B1822);
    fill(12, HEIGHT - 43, 34, 2, 0xFF5874);
    circle(29, HEIGHT - 27, 9, 0xFF5874);
    fill(27, HEIGHT - 39, 5, 13, 0x2B1822);
    line(29, HEIGHT - 40, 29, HEIGHT - 27, 0xFF5874);
    for (window = 0; window < MAX_WINDOWS; ++window) {
        if (window != active_window) {
            draw_window(&windows[window], window);
        }
    }
    if (active_window >= 0) {
        draw_window(&windows[active_window], active_window);
    }
    for (window = 0; window < MAX_WINDOWS; ++window) {
        draw_task_button(&windows[window], window);
    }

    if (cursor_ready) {
        cursor_draw(mouse_x, mouse_y);
    }

    copy_words(screen_fb, backbuffer, WIDTH * HEIGHT);
    fb = screen_fb;
    redraw_has_cursor = cursor_ready;
}

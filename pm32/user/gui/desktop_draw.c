static void redraw(void)
{
    int window;

    fb = backbuffer;
    fill(0, 0, WIDTH, HEIGHT, 0x000000);
    replay_native_draw_layer(0);
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

    replay_native_draw_layer(1);
    replay_native_draw_layer(2);

    copy_words(screen_fb, backbuffer, WIDTH * HEIGHT);
    fb = screen_fb;
    redraw_has_cursor = 0;
}

static void gui_main(void)
{
    int window;
    int x = 640;
    int y = 360;
    int oldx = x;
    int oldy = y;
    int drawn = 0;
    int delivered_left = 0;
    u8 packet[3];
    u8 value;
    int raw;

    for (window = 0; window < MAX_WINDOWS; ++window) {
        windows[window].pid = -1;
    }
    icon_size = sys_read_file(&icon_request);
    if (icon_size > 0) decode_png(icon_png, (unsigned)icon_size);
    set_video_mode();
    redraw();
    mouse_x = x;
    mouse_y = y;
    redraw();
    desktop_pid = sys_spawn_gui("jsgui desktop.js");
    drawn = 1;
    for (;;) {
        raw = sys_input_read();
        if (!raw) {
            process_native_draw_commands();
            continue;
        }
        if (!(raw & 0x01000000UL)) {
            value = (u8)raw;
            if (!(value & 0x80)) {
                if (active_window >= 0 &&
                    !windows[active_window].minimized &&
                    (value == 0x48 || value == 0x49)) {
                    terminal_scroll(&windows[active_window],
                                    value == 0x49 ? -10 : -1);
                } else if (active_window >= 0 &&
                           !windows[active_window].minimized &&
                           (value == 0x50 || value == 0x51)) {
                    terminal_scroll(&windows[active_window],
                                    value == 0x51 ? 10 : 1);
                } else handle_keyboard_scan(value);
            } else handle_keyboard_scan(value);
            if (active_window >= 0 && windows[active_window].visible &&
                !windows[active_window].minimized) {
                redraw();
            }
            process_native_draw_commands();
            continue;
        }
        packet[0] = (u8)raw;
        packet[1] = (u8)(raw >> 8);
        packet[2] = (u8)(raw >> 16);
        if ((packet[0] & 0xC0) ||
            (((packet[0] & 0x10) != 0) != ((packet[1] & 0x80) != 0)) ||
            (((packet[0] & 0x20) != 0) != ((packet[2] & 0x80) != 0))) {
            continue;
        }
        redraw_has_cursor = 0;
        x += (signed char)packet[1];
        y -= (signed char)packet[2];
        if (x < 0) {
            x = 0;
        }
        if (x > WIDTH - 13) {
            x = WIDTH - 13;
        }
        if (y < 0) {
            y = 0;
        }
        if (y > HEIGHT - 19) {
            y = HEIGHT - 19;
        }
        mouse_x = x;
        mouse_y = y;
        if (desktop_pid > 0 && (packet[1] != 0 || packet[2] != 0)) {
            GuiEvent event;
            event.type = GUI_EVENT_MOUSE;
            event.x = x;
            event.y = y;
            event.value = delivered_left;
            sys_gui_send_event((unsigned)desktop_pid, &event);
        }
        if ((packet[0] & 1) && !delivered_left &&
            desktop_pid > 0) {
            GuiEvent event;
            event.type = GUI_EVENT_MOUSE_BUTTON;
            event.x = x;
            event.y = y;
            event.value = 1;
            sys_gui_send_event((unsigned)desktop_pid, &event);
            delivered_left = 1;
        } else if (!(packet[0] & 1) && delivered_left && desktop_pid > 0) {
            GuiEvent event;
            event.type = GUI_EVENT_MOUSE_BUTTON;
            event.x = x;
            event.y = y;
            event.value = GUI_BUTTON_RELEASE;
            sys_gui_send_event((unsigned)desktop_pid, &event);
            delivered_left = 0;
        }
        oldx = x;
        oldy = y;
        drawn = 1;
        /* Do not starve the JS renderer while PS/2 packets keep arriving. */
        process_native_draw_commands();
    }
}

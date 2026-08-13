static void gui_main(void)
{
    int window;
    int x = 640;
    int y = 360;
    int oldx = x;
    int oldy = y;
    int pos = 0;
    int drawn = 0;
    int left = 0;
    u8 packet[3];
    u8 status;
    u8 value;

    for (window = 0; window < MAX_WINDOWS; ++window) {
        windows[window].pid = -1;
    }
    icon_size = sys_read_file(&icon_request);
    if (icon_size > 0) {
        decode_png(icon_png, (unsigned)icon_size);
    }
    set_video_mode();
    redraw();
    mouse_init();
    cursor_ready = 1;
    mouse_x = x;
    mouse_y = y;
    redraw();
    drawn = 1;
    for (;;) {
        status = in8(0x64);
        if (!(status & 1)) {
            sys_yield();
            accept_gui_windows();
            for (window = 0; window < MAX_WINDOWS; ++window) {
                read_shell_output(&windows[window]);
            }
            if (active_window >= 0 &&
                windows[active_window].visible &&
                !windows[active_window].minimized &&
                ++blink_ticks >= 12000) {
                blink_ticks = 0;
                caret_visible = !caret_visible;
                windows[active_window].refresh = 1;
            }
            for (window = 0; window < MAX_WINDOWS; ++window) {
                if (!windows[window].refresh) {
                    continue;
                }
                redraw();
                windows[window].refresh = 0;
                break;
            }
            continue;
        }
        value = in8(0x60);
        if (!(status & 0x20)) {
            /* A keyboard byte interrupts an incomplete mouse packet. */
            pos = 0;
            if (value == 0x2A || value == 0x36) {
                keyboard_shift = 1;
            } else if (value == 0xAA || value == 0xB6) {
                keyboard_shift = 0;
            } else if (!(value & 0x80)) {
                if (active_window < 0 && value == 0x14) {
                    for (window = 0; window < MAX_WINDOWS; ++window) {
                        if (!windows[window].visible) {
                            windows[window].x = 180 + window * 90;
                            windows[window].y = 100 + window * 55;
                            windows[window].visible = 1;
                            windows[window].minimized = 0;
                            windows[window].terminal_size = 0;
                            windows[window].scroll_line = 0;
                            windows[window].app_type = APP_CONSOLE;
                            windows[window].pid = sys_spawn("shell");
                            active_window = window;
                            sys_yield();
                            read_shell_output(&windows[window]);
                            break;
                        }
                    }
                } else if (active_window < 0 && value == 0x32) {
                    sys_spawn_gui("jsgui monitor.js");
                    sys_yield();
                    accept_gui_windows();
                } else if (active_window >= 0 &&
                    !windows[active_window].minimized &&
                    (value == 0x48 || value == 0x49)) {
                    terminal_scroll(&windows[active_window],
                                    value == 0x49 ? -10 : -1);
                } else if (active_window >= 0 &&
                           !windows[active_window].minimized &&
                           (value == 0x50 || value == 0x51)) {
                    terminal_scroll(&windows[active_window],
                                    value == 0x51 ? 10 : 1);
                } else if (value == 0x39)
                    handle_key(' ');
                else if (value < 128 && key_ascii[value])
                    handle_key(keyboard_shift
                                   ? key_ascii_shift[value]
                                   : key_ascii[value]);
            }
            if (active_window >= 0 && windows[active_window].visible &&
                !windows[active_window].minimized) {
                redraw();
            }
            continue;
        }
        if (pos == 0) {
            /* Bit 3 is always set in the first byte of a PS/2 packet. */
            if (!(value & 8)) {
                continue;
            }
            packet[0] = value;
            pos = 1;
            continue;
        }
        packet[pos] = value;
        ++pos;
        if (pos != 3) {
            continue;
        }
        pos = 0;
        if (packet[0] & 0xC0) {
            continue;
        }
        if (drawn) {
            cursor_restore(oldx, oldy);
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
        if ((packet[0] & 1) && !left) {
            click(x, y);
        }
        left = (packet[0] & 1) != 0;
        if (!left) {
            dragging = 0;
            dragging_window = -1;
        }
        if (dragging && dragging_window >= 0) {
            TerminalWindow *window_to_move = &windows[dragging_window];

            window_to_move->x = x - drag_dx;
            window_to_move->y = y - drag_dy;
            if (window_to_move->x < 0) {
                window_to_move->x = 0;
            }
            if (window_to_move->x > WIDTH - 800) {
                window_to_move->x = WIDTH - 800;
            }
            if (window_to_move->y < 0) {
                window_to_move->y = 0;
            }
            if (window_to_move->y > HEIGHT - 480) {
                window_to_move->y = HEIGHT - 480;
            }
            redraw();
        }
        oldx = x;
        oldy = y;
        if (!redraw_has_cursor) {
            cursor_draw(x, y);
        }
        drawn = 1;
        sys_yield();
    }
}

static void click(int x, int y)
{
    int index;
    int pass;
    int hit_order[MAX_WINDOWS];
    int task_x;
    TerminalWindow *window;

    if (y >= HEIGHT - 42 && y < HEIGHT - 10) {
        for (index = 0; index < MAX_WINDOWS; ++index) {
            task_x = 58 + index * 184;
            if (windows[index].visible &&
                x >= task_x && x < task_x + 174) {
                windows[index].minimized = 0;
                active_window = index;
                redraw();
                return;
            }
        }
    }
    if (active_window >= 0) {
        hit_order[0] = active_window;
        pass = 1;
        for (index = MAX_WINDOWS - 1; index >= 0; --index) {
            if (index != active_window) {
                hit_order[pass++] = index;
            }
        }
    } else {
        for (index = 0; index < MAX_WINDOWS; ++index) {
            hit_order[index] = MAX_WINDOWS - index - 1;
        }
    }

    for (pass = 0; pass < MAX_WINDOWS; ++pass) {
        index = hit_order[pass];
        window = &windows[index];

        if (!window->visible) {
            continue;
        }
        if (window->minimized) {
            continue;
        }
        if (x >= window->x + 752 && x < window->x + 800 &&
            y >= window->y && y < window->y + 34) {
            sys_kill((unsigned)window->pid);
            window->visible = 0;
            window->pid = -1;
            if (active_window == index) {
                active_window = -1;
                for (pass = MAX_WINDOWS - 1; pass >= 0; --pass) {
                    if (windows[pass].visible &&
                        !windows[pass].minimized) {
                        active_window = pass;
                        break;
                    }
                }
            }
            redraw();
            return;
        }
        if (x >= window->x + 710 && x < window->x + 750 &&
            y >= window->y && y < window->y + 34) {
            window->minimized = 1;
            if (active_window == index) {
                active_window = -1;
                for (pass = MAX_WINDOWS - 1; pass >= 0; --pass) {
                    if (windows[pass].visible &&
                        !windows[pass].minimized && pass != index) {
                        active_window = pass;
                        break;
                    }
                }
            }
            redraw();
            return;
        }
        if (x >= window->x && x < window->x + 800 &&
            y >= window->y && y < window->y + 480) {
            active_window = index;
            if (y < window->y + 34) {
                dragging = 1;
                dragging_window = index;
                drag_dx = x - window->x;
                drag_dy = y - window->y;
            }
            redraw();
            return;
        }
    }
}

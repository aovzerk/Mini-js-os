static int terminal_line_count(const TerminalWindow *window)
{
    int i;
    int lines = 1;
    int column = 0;

    for (i = 0; i < window->terminal_size; ++i) {
        if (window->terminal[i] == '\n') {
            ++lines;
            column = 0;
        } else if (window->terminal[i] == '\b') {
            if (column > 0) {
                --column;
            }
        } else if (++column >= TERMINAL_COLUMNS) {
            ++lines;
            column = 0;
        }
    }
    return lines;
}

static void terminal_scroll(TerminalWindow *window, int amount)
{
    int maximum = terminal_line_count(window) - TERMINAL_VISIBLE_LINES;

    if (maximum < 0) {
        maximum = 0;
    }
    window->scroll_line += amount;
    if (window->scroll_line < 0) {
        window->scroll_line = 0;
    }
    if (window->scroll_line > maximum) {
        window->scroll_line = maximum;
    }
    window->refresh = 1;
}

static void draw_terminal_area(TerminalWindow *window)
{
    if (!window->visible || window->minimized) {
        return;
    }
    fill(window->x + 18, window->y + 52, 764, 397, 0x071019);
    fill(window->x + 18, window->y + 52, 764, 1, 0x1B4054);
    fill(window->x + 18, window->y + 448, 764, 1, 0x102C3C);
    draw_terminal(window);
}

static void terminal_append(TerminalWindow *window,
                            const char *text,
                            unsigned length)
{
    unsigned i;

    for (i = 0; i < length; ++i) {
        if (text[i] == '\b') {
            if (window->terminal_size > 0) {
                --window->terminal_size;
            }
        } else if (text[i] == '\f') {
            window->terminal_size = 0;
            window->scroll_line = 0;
        } else if (window->terminal_size < TERMINAL_BUFFER) {
            window->terminal[window->terminal_size++] = text[i];
        }
    }
    window->scroll_line = terminal_line_count(window) -
                          TERMINAL_VISIBLE_LINES;
    if (window->scroll_line < 0) {
        window->scroll_line = 0;
    }
    window->refresh = 1;
}

static void read_shell_output(TerminalWindow *window)
{
    int length;

    if (!window->visible || window->pid < 0) {
        return;
    }
    length = sys_terminal_read((unsigned)window->pid, child_output);

    if (length > 0) {
        terminal_append(window, child_output, (unsigned)length);
    }
}

static void accept_gui_windows(void)
{
    int pid;

    while ((pid = sys_gui_next_window()) >= 0) {
        int index;

        for (index = 0; index < MAX_WINDOWS; ++index) {
            if (!windows[index].visible) {
                windows[index].x = 180 + index * 90;
                windows[index].y = 100 + index * 55;
                windows[index].visible = 1;
                windows[index].minimized = 0;
                windows[index].terminal_size = 0;
                windows[index].scroll_line = 0;
                windows[index].app_type = APP_GUI;
                windows[index].pid = pid;
                if (sys_gui_get_title((unsigned)pid, windows[index].title) < 0)
                    windows[index].title[0] = 0;
                windows[index].refresh = 1;
                active_window = index;
                read_shell_output(&windows[index]);
                break;
            }
        }
        if (index == MAX_WINDOWS) {
            sys_kill((unsigned)pid);
        }
    }
}
static void draw_window(TerminalWindow *window, int index)
{
    if (!window->visible) {
        return;
    }
    if (window->minimized) {
        return;
    }
    if (window->x < 0) {
        window->x = 0;
    }
    if (window->x > WIDTH - 800) {
        window->x = WIDTH - 800;
    }
    if (window->y < 0) {
        window->y = 0;
    }
    if (window->y > HEIGHT - 480) {
        window->y = HEIGHT - 480;
    }
    fill(window->x + 8, window->y + 10, 800, 480, 0x071019);
    fill(window->x, window->y, 800, 480, 0x10212E);
    fill(window->x, window->y, 800, 2,
         index == active_window ? 0x20D7E5 : 0x1B4054);
    fill(window->x, window->y + 2, 800, 40, 0x0C1B27);
    fill(window->x + 16, window->y + 13, 14, 14, 0x20D7E5);
    fill(window->x + 20, window->y + 17, 6, 6, 0x071019);
    draw_text(window->x + 40, window->y + 12, "MYOS //");
    draw_text(window->x + 112, window->y + 12,
              window->app_type == APP_GUI && window->title[0]
                  ? window->title : "TERMINAL");
    fill(window->x + 710, window->y + 7, 36, 28, 0x142C3B);
    line(window->x + 721, window->y + 23,
         window->x + 735, window->y + 23, 0x8CAAB8);
    fill(window->x + 752, window->y + 7, 40, 28, 0x2B1822);
    line(window->x + 765, window->y + 15,
         window->x + 779, window->y + 27, 0xFF5874);
    line(window->x + 779, window->y + 15,
         window->x + 765, window->y + 27, 0xFF5874);
    draw_terminal_area(window);
}

#include "taskbar.c"

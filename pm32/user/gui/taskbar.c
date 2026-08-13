/* Included by gui.c after the drawing primitives and TerminalWindow. */
static void draw_task_button(TerminalWindow *window, int index)
{
    int task_x = 58 + index * 184;
    u32 background;
    u32 accent;

    if (!window->visible) return;
    if (index == active_window && !window->minimized) {
        background = 0x15394A;
        accent = 0x20D7E5;
    } else if (window->minimized) {
        background = 0x0B1D29;
        accent = 0x466273;
    } else {
        background = 0x102B3B;
        accent = 0x1B4054;
    }
    fill(task_x, HEIGHT - 42, 174, 32, background);
    fill(task_x, HEIGHT - 42, 174, 2, accent);
    fill(task_x + 11, HEIGHT - 33, 14, 14, accent);
    draw_text(task_x + 34, HEIGHT - 34,
              window->app_type == APP_GUI && window->title[0]
                  ? window->title : "TERMINAL");
}

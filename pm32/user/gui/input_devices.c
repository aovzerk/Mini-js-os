static const u16 arrow[18] = {
    1, 3, 7, 15, 31, 63, 127, 255, 511,
    1023, 2047, 4095, 511, 459, 897, 896, 1792, 1792
};
static u32 under[12*18];
static int dragging = 0;
static int drag_dx;
static int drag_dy;
static int mouse_x = 640;
static int mouse_y = 360;
static int cursor_ready;
static int redraw_has_cursor;
static void cursor_restore(int x, int y)
{
    int yy;
    int xx;
    int n = 0;

    for (yy = 0; yy < 18; ++yy) {
        for (xx = 0; xx < 12; ++xx) {
            if (x + xx >= 0 && x + xx < WIDTH &&
                y + yy >= 0 && y + yy < HEIGHT)
                fb[(y + yy) * WIDTH + x + xx] = under[n];
            ++n;
        }
    }
}

static void cursor_draw(int x, int y)
{
    int yy;
    int xx;
    int n = 0;

    for (yy = 0; yy < 18; ++yy) {
        for (xx = 0; xx < 12; ++xx) {
            if (x + xx >= 0 && x + xx < WIDTH &&
                y + yy >= 0 && y + yy < HEIGHT) {
                volatile u32 *pixel = &fb[(y + yy) * WIDTH + x + xx];
                under[n] = *pixel;
                if (arrow[yy] & (1 << xx))
                    *pixel = (xx == 0 || yy == 0 ||
                              !(arrow[yy] & (1 << (xx + 1))))
                                 ? 0x000000 : 0xFFFFFF;
            }
            ++n;
        }
    }
}

static void set_register(u16 index, u16 value)
{
    out16(0x1CE, index);
    out16(0x1CF, value);
}
static void set_video_mode(void)
{
    out32(0xCF8, 0x80001010UL);
    screen_fb = (volatile u32 *)(in32(0xCFC) & 0xFFFFFFF0UL);
    fb = screen_fb;
    set_register(4, 0);
    set_register(1, WIDTH);
    set_register(2, HEIGHT);
    set_register(3, 32);
    set_register(4, 0x41);
}
static void fill(int x, int y, int w, int h, u32 color)
{
    int yy;
    int x2;
    int y2;

    if (w <= 0 || h <= 0)
        return;
    x2 = x + w;
    y2 = y + h;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x2 > WIDTH) {
        x2 = WIDTH;
    }
    if (y2 > HEIGHT) {
        y2 = HEIGHT;
    }
    if (x >= x2 || y >= y2)
        return;
    for (yy = y; yy < y2; ++yy) {
        fill_words(&fb[yy * WIDTH + x], color, (unsigned)(x2 - x));
    }
}

static void line(int x0, int y0, int x1, int y1, u32 color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        int doubled_error;

        fill(x0, y0, 1, 1, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        doubled_error = error * 2;
        if (doubled_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void circle(int center_x, int center_y, int radius, u32 color)
{
    int x = radius;
    int y = 0;
    int error = 1 - radius;

    while (x >= y) {
        fill(center_x + x, center_y + y, 1, 1, color);
        fill(center_x + y, center_y + x, 1, 1, color);
        fill(center_x - y, center_y + x, 1, 1, color);
        fill(center_x - x, center_y + y, 1, 1, color);
        fill(center_x - x, center_y - y, 1, 1, color);
        fill(center_x - y, center_y - x, 1, 1, color);
        fill(center_x + y, center_y - x, 1, 1, color);
        fill(center_x + x, center_y - y, 1, 1, color);
        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x) + 1;
        }
    }
}
static u32 be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) |
           ((u32)p[2] << 8) | p[3];
}

/* PNG subset: RGBA8, filter 0, one zlib stored block. */
static int decode_png(const u8 *png, unsigned size)
{
    unsigned p = 8;
    unsigned w = 0;
    unsigned h = 0;
    unsigned x;
    unsigned y;
    unsigned len;
    const u8 *data = 0;
    if (size < 33 || png[0] != 137 || png[1] != 'P' ||
        png[2] != 'N' || png[3] != 'G') {
        return 0;
    }
    while (p + 12 <= size) {
        u32 n = be32(png + p);
        const u8 *type = png + p + 4;
        const u8 *chunk = png + p + 8;

        if (p + 12 + n > size) {
            return 0;
        }
        if (type[0] == 'I' && type[1] == 'H') {
            w = be32(chunk);
            h = be32(chunk + 4);
            if (chunk[8] != 8 || chunk[9] != 6) {
                return 0;
            }
        }
        if (type[0] == 'I' && type[1] == 'D') {
            data = chunk;
            len = n;
            break;
        }
        p += 12 + n;
    }
    if (!data || len < 7 || w > 48 || h > 48 || data[2] != 1) {
        return 0;
    }
    {
      const u8 *raw = data + 7;
      unsigned expected = h * (1 + w * 4);
      u32 row[48];

      if (expected + 11 > len) {
          return 0;
      }
      for (y = 0; y < h; ++y) {
          if (*raw++) {
              return 0;
          }
          for (x = 0; x < w; ++x) {
              u8 r = *raw++;
              u8 g = *raw++;
              u8 b = *raw++;
              u8 a = *raw++;
              row[x] = ((u32)a << 24) | ((u32)r << 16) |
                       ((u32)g << 8) | b;
          }
          for (x = 0; x < w; ++x)
              ((u32 *)icon_png)[y * 48 + x] = row[x];
      }
    }
    icon_width = w;
    icon_height = h;
    return 1;
}
static void draw_icon(int left, int top)
{
    unsigned x;
    unsigned y;
    u32 *pixels = (u32 *)icon_png;

    for (y = 0; y < icon_height; ++y) {
        for (x = 0; x < icon_width; ++x) {
            u32 color = pixels[y * 48 + x];
            u32 alpha = color >> 24;
            volatile u32 *destination;

            if (!alpha) {
                continue;
            }
            destination = &fb[(top + y) * WIDTH + left + x];
            if (alpha == 255) {
                *destination = color & 0xFFFFFF;
            } else {
                u32 old = *destination;
                u32 red = (((color >> 16) & 255) * alpha +
                           ((old >> 16) & 255) * (255 - alpha)) / 255;
                u32 green = (((color >> 8) & 255) * alpha +
                             ((old >> 8) & 255) * (255 - alpha)) / 255;
                u32 blue = ((color & 255) * alpha +
                            (old & 255) * (255 - alpha)) / 255;
                *destination = (red << 16) | (green << 8) | blue;
            }
        }
    }
}

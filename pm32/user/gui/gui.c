#include <myos/api.h>
#include <myos/gui.h>

#define WIDTH 1280
#define HEIGHT 720
#define BACKBUFFER_ADDRESS 0x400000UL

static volatile u32 *fb;
static volatile u32 *screen_fb;
static volatile u32 *backbuffer =
    (volatile u32 *)BACKBUFFER_ADDRESS;
static u8 icon_png[10000];
static const char icon_name[11] = {
    'S', 'H', 'E', 'L', 'L', 'I', 'C', 'O', 'P', 'N', 'G'
};
static FileRequest icon_request={icon_name,icon_png,sizeof(icon_png)};
static int icon_size;
static char child_output[256];
static unsigned icon_width;
static unsigned icon_height;
static unsigned blink_ticks;
static int caret_visible=1;
static int keyboard_shift;

#define MAX_WINDOWS 3
#define TERMINAL_BUFFER 4096
#define TERMINAL_COLUMNS 92
#define TERMINAL_VISIBLE_LINES 21

typedef struct TerminalWindow {
    int visible;
    int minimized;
    int x;
    int y;
    int pid;
    int refresh;
    int app_type;
    int terminal_size;
    int scroll_line;
    char title[32];
    char terminal[TERMINAL_BUFFER];
} TerminalWindow;

static TerminalWindow windows[MAX_WINDOWS];
static int active_window = -1;
static int dragging_window = -1;

static u8 in8(u16 port);
static void out8(u16 port, u8 value);
static void out16(u16 port, u16 value);
static u32 in32(u16 port);
static void out32(u16 port, u32 value);
static void fill_words(volatile u32 *destination, u32 color, unsigned count);
static void copy_words(volatile u32 *destination,
                       const volatile u32 *source,
                       unsigned count);
#pragma aux in8 = "in al,dx" parm [dx] value [al] modify exact [al];
#pragma aux out8 = "out dx,al" parm [dx] [al] modify exact [];
#pragma aux out16 = "out dx,ax" parm [dx] [ax] modify exact [];
#pragma aux in32 = "in eax,dx" parm [dx] value [eax] modify exact [eax];
#pragma aux out32 = "out dx,eax" parm [dx] [eax] modify exact [];
#pragma aux fill_words = \
    "cld" "rep stosd" \
    parm [edi] [eax] [ecx] modify exact [edi ecx];
#pragma aux copy_words = \
    "cld" "rep movsd" \
    parm [edi] [esi] [ecx] modify exact [edi esi ecx];

static void gui_main(void);
void _start(void){gui_main();}

#include "video.c"
#include "input_devices.c"
#include "terminal.c"
#include "desktop.c"
#include "event_loop.c"

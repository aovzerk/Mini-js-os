#include <myos/types.h>

/* Keep the freestanding kernel as one flat code/data image. */
#pragma data_seg("_TEXT", "CODE")
#pragma const_seg("_TEXT", "CODE")
#pragma bss_seg("_BSS", "BSS")

#define USER_ARGS   ((char *)0x5FF000UL)
#define USER_STACK  0x600000UL
#define USER_HEAP_BASE 0x500000UL
#define USER_HEAP_SIZE 0x1F000UL
#define DISK_CACHE  ((const u8 *)0x40000UL)
#define VGA         ((volatile u16 *)0xB8000UL)
#define MAX_PROCESSES 8
#define USER_BASE 0x400000UL
#define USER_IMAGE_SIZE 0x20000UL
#define USER_SIZE 0x80000UL
#define PAGE_DIRECTORY_BASE 0x80000UL
#define PAGE_TABLE_BASE 0x90000UL
#define PROCESS_MEMORY_PHYSICAL_BASE 0x800000UL
/* Supervisor-only alias used after paging is enabled.  Virtual 0x800000 is
   reserved for the GUI backbuffer and must never be used to access process
   images from the kernel. */
#define PROCESS_MEMORY_BASE 0x1000000UL
#define TERMINAL_CAPACITY 256
#define TERMINAL_HISTORY_CAPACITY 4096
#define CONSOLE_HISTORY_ROWS 64
#define GUI_EVENT_QUEUE_SIZE 16
#define GUI_DRAW_QUEUE_SIZE 16
#define FAT_LBA 128UL
#define FAT_DATA_LBA (FAT_LBA + 2UL * 1009UL)
#define KERNEL_BOOT_INIT 0xFFFFFFFFUL

typedef struct KernelRequest {
    u32 number;
    u32 arg0;
    u32 arg1;
    u32 user_esp;
    u32 user_eip;
    u32 result;
    u32 next_esp;
    u32 next_eip;
    u32 next_cr3;
    u32 next_ebx;
    u32 next_esi;
    u32 next_ebp;
    u32 next_edi;
    u32 saved_eax;
    u32 saved_ecx;
    u32 saved_edx;
    u32 next_ecx;
    u32 next_edx;
} KernelRequest;
typedef struct Process {
    u32 active;
    u32 started;
    u32 esp;
    u32 eip;
    u32 cr3;
    u32 ebx;
    u32 esi;
    u32 ebp;
    u32 edi;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 terminal_pid;
    u32 parent_pid;
    u32 waiting_child;
    u32 sleeping_until;
    u32 app_type;
    u32 gui_window_requested;
    char name[9];
    char gui_title[32];
    u32 gui_draw_head;
    u32 gui_draw_tail;
    GuiDrawCommand gui_draw_queue[GUI_DRAW_QUEUE_SIZE];
    u32 gui_event_head;
    u32 gui_event_tail;
    GuiEvent gui_events[GUI_EVENT_QUEUE_SIZE];
} Process;

static u8 in8(u16 port);
static void out8(u16 port, u8 value);
static u16 in16(u16 port);
static void out16(u16 port, u16 value);
static void cpu_halt(void);
static void cpu_wait_interrupt(void);
#pragma aux in8 = "in al,dx" parm [dx] value [al] modify exact [al];
#pragma aux out8 = "out dx,al" parm [dx] [al] modify exact [];
#pragma aux in16 = "in ax,dx" parm [dx] value [ax] modify exact [ax];
#pragma aux out16 = "out dx,ax" parm [dx] [ax] modify exact [];
#pragma aux cpu_halt = "cli" "hlt" modify exact [];
#pragma aux cpu_wait_interrupt = "sti" "hlt" "cli" modify exact [];

static unsigned cursor;
static u16 console_history[CONSOLE_HISTORY_ROWS * 80];
static unsigned console_row;
static unsigned console_column;
static unsigned console_view_offset;
static Process processes[MAX_PROCESSES];
static unsigned current_process;
static unsigned gui_focus_pid;
static u8 key_queue[MAX_PROCESSES][64];
static unsigned key_head[MAX_PROCESSES];
static unsigned key_tail[MAX_PROCESSES];
static u8 terminal_output[MAX_PROCESSES][TERMINAL_CAPACITY];
static unsigned terminal_size[MAX_PROCESSES];
static unsigned terminal_history_size[MAX_PROCESSES];
static u8 sector_buffer[512];
static const u8 scan_ascii[128] = {
  0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,0,
  'q','w','e','r','t','y','u','i','o','p','[',']',13,0,
  'a','s','d','f','g','h','j','k','l',';',39,'`',0,'\\',
  'z','x','c','v','b','n','m',',','.','/'
};
static const u8 scan_ascii_shift[128] = {
  0,27,'!','@','#','$','%','^','&','*','(',')','_','+',8,0,
  'Q','W','E','R','T','Y','U','I','O','P','{','}',13,0,
  'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
  'Z','X','C','V','B','N','M','<','>','?'
};
static int keyboard_shift;
static int keyboard_extended;

static void update_cursor(void);
static void console_clear(void);
static void console_init(void);
static void console_write(const char *text, unsigned length);
static void console_page_up(void);
static void console_page_down(void);
static int load_program(const char name[11]);
static int load_file(const char name[11], u8 *destination, u32 capacity);
static int file_app_type(const char name[11]);
static int list_files(char *destination, unsigned capacity);
static int write_file(const char name[11], const u8 *source, u32 size);
static int ata_write_sector(u32 lba, const u8 *source);
static int ata_read_sector(u32 lba, u8 *destination);
static int execute(const char *command);
static int read_key(void);
static int ps2_read_keyboard_byte(u8 *value);
static int ps2_read_mouse_packet(u32 *packet);
static int mouse_read_packet(u32 *packet);
static u8 *bootstrap_user_memory(void);
static u8 *user_memory(void);
static void make_address_space(unsigned pid);
static void set_process_name(unsigned pid, const char name[11]);
static void fill_process_info(ProcessInfo *info, unsigned pid);
static int spawn_process(const char name[11]);
static int spawn_command(const char *command);
static void switch_process(KernelRequest *r, unsigned next);
static void schedule(KernelRequest *r);
static void heap_reset(unsigned pid);
static void heap_reset_at(u32 base, unsigned pid);
static void *heap_allocate(u32 size);
static int heap_release(void *pointer);
static u32 clock_millis(void);
static void terminal_history_append(unsigned channel,
                                    const u8 *text, unsigned length);
static unsigned terminal_history_copy(unsigned process, u8 *destination);
static int dispatch_input_syscall(KernelRequest *r);
static int dispatch_time_syscall(KernelRequest *r);
static int dispatch_memory_syscall(KernelRequest *r);
static int dispatch_gui_ipc_syscall(KernelRequest *r);
static u32 kernel_dispatch_impl(KernelRequest *r);

#include "kernel_entry.c"
#include "input/ps2.c"
#include "input/keyboard.c"
#include "input/mouse.c"
#include "syscall_input.c"
#include "syscall_time.c"
#include "syscall_memory.c"
#include "syscall_gui_ipc.c"
#include "dispatch.c"
#include "drivers/vga_console.c"
#include "filesystem.c"
#include "processes.c"
#include "heap.c"
#include "clock.c"

#include <myos/types.h>

/* Keep the freestanding kernel as one flat code/data image. */
#pragma data_seg("_TEXT", "CODE")
#pragma const_seg("_TEXT", "CODE")
#pragma bss_seg("_TEXT", "CODE")

#define USER_ARGS   ((char *)0x2E000UL)
#define USER_STACK  0x2F000UL
#define DISK_CACHE  ((const u8 *)0x40000UL)
#define VGA         ((volatile u16 *)0xB8000UL)
#define MAX_PROCESSES 8
#define USER_BASE 0x20000UL
#define USER_SIZE 0x20000UL
#define PAGE_DIRECTORY_BASE 0x80000UL
#define PAGE_TABLE_BASE 0x90000UL
#define PROCESS_MEMORY_BASE 0x100000UL
#define TERMINAL_CAPACITY 256
#define FAT_DATA_LBA (64UL + 2UL * 1009UL)

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
    u32 terminal_pid;
    u32 parent_pid;
    u32 waiting_child;
    u32 app_type;
    u32 gui_window_requested;
    char name[9];
    char gui_title[32];
} Process;

static u8 in8(u16 port);
static void out8(u16 port, u8 value);
static u16 in16(u16 port);
static void out16(u16 port, u16 value);
static void cpu_halt(void);
#pragma aux in8 = "in al,dx" parm [dx] value [al] modify exact [al];
#pragma aux out8 = "out dx,al" parm [dx] [al] modify exact [];
#pragma aux in16 = "in ax,dx" parm [dx] value [ax] modify exact [ax];
#pragma aux out16 = "out dx,ax" parm [dx] [ax] modify exact [];
#pragma aux cpu_halt = "cli" "hlt" modify exact [];

static unsigned cursor;
static Process processes[MAX_PROCESSES];
static unsigned current_process;
static u8 key_queue[MAX_PROCESSES][64];
static unsigned key_head[MAX_PROCESSES];
static unsigned key_tail[MAX_PROCESSES];
static u8 terminal_output[MAX_PROCESSES][TERMINAL_CAPACITY];
static unsigned terminal_size[MAX_PROCESSES];
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

static void update_cursor(void);
static void console_clear(void);
static void console_write(const char *text, unsigned length);
static int load_program(const char name[11]);
static int load_file(const char name[11], u8 *destination, u32 capacity);
static int file_app_type(const char name[11]);
static int list_files(char *destination, unsigned capacity);
static int write_file(const char name[11], const u8 *source, u32 size);
static int ata_write_sector(u32 lba, const u8 *source);
static int ata_read_sector(u32 lba, u8 *destination);
static int execute(const char *command);
static int read_key(void);
static u8 *user_memory(void);
static void make_address_space(unsigned pid);
static void set_process_name(unsigned pid, const char name[11]);
static void fill_process_info(ProcessInfo *info, unsigned pid);
static int spawn_process(const char name[11]);
static int spawn_command(const char *command);
static void switch_process(KernelRequest *r, unsigned next);
static void schedule(KernelRequest *r);

#include "dispatch.c"
#include "console.c"
#include "filesystem.c"
#include "processes.c"

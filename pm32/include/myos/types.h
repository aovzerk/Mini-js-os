#ifndef MYOS_TYPES_H
#define MYOS_TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define APP_CONSOLE 0
#define APP_GUI 1

typedef struct FileRequest {
    const char *name;
    void *destination;
    u32 capacity;
} FileRequest;

typedef struct WriteRequest {
    const char *name;
    const void *source;
    u32 size;
} WriteRequest;

typedef struct ProcessInfo {
    u32 pid;
    char name[9];
    u32 active;
    u32 app_type;
    u32 parent_pid;
    u32 terminal_pid;
} ProcessInfo;

#define GUI_DRAW_FILL_RECT 1
#define GUI_DRAW_TEXT 2
#define GUI_DRAW_PRESENT 3
#define GUI_DRAW_TERMINAL 4
#define GUI_DRAW_IMAGE 5
#define GUI_DRAW_FOCUS 6
#define GUI_DRAW_CURSOR 7

typedef struct GuiDrawCommand {
    u32 type;
    u32 pid;
    u32 target_pid;
    u32 layer;
    long x;
    long y;
    long width;
    long height;
    u32 color;
    char text[64];
} GuiDrawCommand;

#define GUI_EVENT_KEY 1
#define GUI_EVENT_MOUSE 2
#define GUI_EVENT_MOUSE_BUTTON 3
#define GUI_BUTTON_RELEASE 0
#define GUI_BUTTON_PRESS 1
#define GUI_BUTTON_CANCEL 2

typedef struct GuiEvent {
    u32 type;
    long x;
    long y;
    u32 value;
} GuiEvent;

#endif

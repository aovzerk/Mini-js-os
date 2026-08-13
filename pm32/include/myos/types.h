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

#endif

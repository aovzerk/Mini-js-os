#ifndef MYOS_API_H
#define MYOS_API_H

#include <myos/types.h>

/* A raw program is one flat image: keep constants and writable data with code. */
#pragma data_seg("_TEXT", "CODE")
#pragma const_seg("_TEXT", "CODE")
#pragma bss_seg("_BSS", "BSS")

int sys_write(const char *text, unsigned length);
int sys_exec(const char *command);
int sys_read_key(void);
int sys_read_file(void *request);
void sys_exit(void);
int sys_spawn(const char name[11]);
int sys_run(const char *command);
void sys_yield(void);
void sys_send_key(unsigned pid, unsigned key);
int sys_terminal_read(unsigned pid, char *buffer);
int sys_terminal_snapshot(unsigned pid, char *buffer);
int sys_get_pid(void);
void sys_kill(unsigned pid);
int sys_write_file(void *request);
int sys_get_app_type(void);
int sys_list_files(char *buffer, unsigned capacity);
int sys_list_processes(ProcessInfo *processes, unsigned capacity);
void sys_poweroff(void);
void *sys_malloc(unsigned size);
int sys_free(void *pointer);
unsigned sys_millis(void);
void sys_wait_until(unsigned deadline);

#pragma aux sys_write = \
    "mov eax,1" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ecx edx];
#pragma aux sys_exec = \
    "mov eax,3" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ecx edx];
#pragma aux sys_read_key = \
    "mov eax,4" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_read_file = \
    "mov eax,5" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ecx edx];
#pragma aux sys_exit = \
    "mov eax,2" "int 80h" \
    aborts modify exact [eax ecx edx];
#pragma aux sys_spawn = \
    "mov eax,6" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_run = \
    "mov eax,14" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_yield = \
    "mov eax,7" "int 80h" \
    modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_wait_until = \
    "mov eax,36" "int 80h" \
    parm [ebx] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_send_key = \
    "mov eax,8" "int 80h" \
    parm [ebx] [esi] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_terminal_read = \
    "mov eax,9" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_terminal_snapshot = \
    "mov eax,32" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_get_pid = \
    "mov eax,10" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_kill = \
    "mov eax,11" "int 80h" \
    parm [ebx] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_write_file = \
    "mov eax,15" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_get_app_type = \
    "mov eax,16" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_list_files = \
    "mov eax,18" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_list_processes = \
    "mov eax,22" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_poweroff = \
    "mov eax,19" "int 80h" \
    aborts modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_malloc = \
    "mov eax,25" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_free = \
    "mov eax,26" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_millis = \
    "mov eax,27" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];

#endif

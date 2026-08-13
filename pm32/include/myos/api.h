#ifndef MYOS_API_H
#define MYOS_API_H

#include <myos/types.h>

/* A raw program is one flat image: keep constants and writable data with code. */
#pragma data_seg("_TEXT", "CODE")
#pragma const_seg("_TEXT", "CODE")
#pragma bss_seg("_TEXT", "CODE")

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

#pragma aux sys_write = \
    "mov eax,1" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] value [eax] modify exact [eax ecx edx];
#pragma aux sys_exec = \
    "mov eax,3" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ecx edx];
#pragma aux sys_read_key = \
    "mov eax,4" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_read_file = \
    "mov eax,5" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ecx edx];
#pragma aux sys_exit = \
    "mov eax,2" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    aborts modify exact [eax ecx edx];
#pragma aux sys_spawn = \
    "mov eax,6" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_run = \
    "mov eax,14" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_yield = \
    "mov eax,7" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_send_key = \
    "mov eax,8" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_terminal_read = \
    "mov eax,9" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_get_pid = \
    "mov eax,10" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_kill = \
    "mov eax,11" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_write_file = \
    "mov eax,15" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_get_app_type = \
    "mov eax,16" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_list_files = \
    "mov eax,18" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_list_processes = \
    "mov eax,22" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_poweroff = \
    "mov eax,19" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    aborts modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_malloc = \
    "mov eax,25" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_free = \
    "mov eax,26" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_millis = \
    "mov eax,27" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];

#endif

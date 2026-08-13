#ifndef MYOS_GUI_H
#define MYOS_GUI_H

int sys_spawn_gui(const char *command);
int sys_gui_create_window(void);
int sys_gui_next_window(void);
int sys_gui_set_title(const char *title);
int sys_gui_get_title(unsigned pid, char *title);

#pragma aux sys_spawn_gui = \
    "mov eax,17" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_create_window = \
    "mov eax,20" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_next_window = \
    "mov eax,21" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_set_title = \
    "mov eax,23" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_get_title = \
    "mov eax,24" "mov ecx,esp" "db 0e8h,0,0,0,0" "pop edx" "add edx,6" "db 0fh,34h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];

#endif

#ifndef MYOS_GUI_H
#define MYOS_GUI_H

int sys_spawn_gui(const char *command);
int sys_gui_create_window(void);
int sys_gui_next_window(void);
int sys_gui_set_title(const char *title);
int sys_gui_get_title(unsigned pid, char *title);
int sys_gui_submit_draw(GuiDrawCommand *command);
int sys_gui_next_draw(GuiDrawCommand *command);
int sys_gui_poll_event(GuiEvent *event);
int sys_gui_send_event(unsigned pid, GuiEvent *event);
int sys_gui_set_focus(unsigned pid);
int sys_gui_get_focus(void);
int sys_input_read(void);

#pragma aux sys_spawn_gui = \
    "mov eax,17" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_create_window = \
    "mov eax,20" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_next_window = \
    "mov eax,21" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_set_title = \
    "mov eax,23" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_get_title = \
    "mov eax,24" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_submit_draw = \
    "mov eax,28" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_next_draw = \
    "mov eax,29" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_poll_event = \
    "mov eax,30" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_send_event = \
    "mov eax,31" "int 80h" \
    parm [ebx] [esi] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_set_focus = \
    "mov eax,33" "int 80h" \
    parm [ebx] value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_gui_get_focus = \
    "mov eax,34" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];
#pragma aux sys_input_read = \
    "mov eax,37" "int 80h" \
    value [eax] modify exact [eax ebx ecx edx esi edi];

#endif

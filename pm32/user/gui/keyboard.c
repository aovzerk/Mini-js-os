static const u8 key_ascii[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'
};
static const u8 key_ascii_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'
};
static void handle_key(int key)
{
    int input_pid = sys_gui_get_focus();
    if (input_pid > 0) {
        native_terminal_echo((u32)input_pid, (u8)key);
        sys_send_key((unsigned)input_pid, (unsigned)key);
    } else if (desktop_pid > 0) {
        GuiEvent event;
        event.type = GUI_EVENT_KEY;
        event.x = 0;
        event.y = 0;
        event.value = (u32)key;
        sys_gui_send_event((unsigned)desktop_pid, &event);
    }
}

static void handle_keyboard_scan(u8 value)
{
    if (value == 0x2A || value == 0x36) keyboard_shift = 1;
    else if (value == 0xAA || value == 0xB6) keyboard_shift = 0;
    else if (!(value & 0x80)) {
        if (value == 0x39) handle_key(' ');
        else if (value < 128 && key_ascii[value])
            handle_key(keyboard_shift ? key_ascii_shift[value]
                                      : key_ascii[value]);
    }
}

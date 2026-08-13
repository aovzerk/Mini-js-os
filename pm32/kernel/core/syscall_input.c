static int dispatch_input_syscall(KernelRequest *r)
{
    if (r->number == 37) {
        u8 key;
        u32 packet;
        if (ps2_read_keyboard_byte(&key)) r->result = key;
        else if (mouse_read_packet(&packet)) r->result = packet;
        else {
            r->result = 0;
        }
        return 1;
    }
    return 0;
}

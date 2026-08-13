#include <myos/api.h>
#include <myos/userlib.h>

#define COMMAND ((char *)0x5FE000UL)
#define MAX_COMMAND 63

void _start(void)
{
    unsigned length;
    int key;
    int graphical = sys_get_pid() != 0;

    for (;;) {
        myos_write_text("> ");
        length = 0;
        for (;;) {
            key = sys_read_key();
            if (!key) {
                continue;
            }
            if (key == '\r' || key == '\n') {
                if (!graphical) sys_write("\n", 1);
                break;
            }
            if (key == '\b') {
                if (length) {
                    --length;
                    if (!graphical) sys_write("\b", 1);
                }
                continue;
            }
            if (key >= 32 && key < 127 && length < MAX_COMMAND) {
                COMMAND[length++] = (char)key;
                if (!graphical) sys_write(&COMMAND[length - 1], 1);
            }
        }
        COMMAND[length] = 0;
        if (length) {
            int result;

            if (COMMAND[0] == 'g' && COMMAND[1] == 'u' &&
                COMMAND[2] == 'i' && COMMAND[3] == 0 &&
                sys_get_pid() == 0) {
                result = sys_exec(COMMAND);
            } else {
                result = sys_run(COMMAND);
            }
            if (result == -3) {
                myos_write_text("program is too large\n");
            } else if (result == -6) {
                myos_write_text("GUI application: start it from GUI\n");
            } else if (result < 0) {
                myos_write_text("program not found\n");
            }
        }
    }
}

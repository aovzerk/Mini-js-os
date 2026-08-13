#include <myos/api.h>

void _start(void)
{
    sys_write("Powering off...\n", 16);
    sys_poweroff();
}

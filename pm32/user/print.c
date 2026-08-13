#include <myos/api.h>

void _start(const char *arguments)
{
    unsigned length = 0;

    while (arguments[length]) {
        ++length;
    }

    sys_write(arguments, length);
    sys_write("\n", 1);
    sys_exit();
}

#pragma aux _start parm [ebx];

#include <myos/api.h>

static char file_list[512];

void _start(void)
{
    int length = sys_list_files(file_list, sizeof(file_list));

    if (length < 0) {
        sys_write("cannot read directory\n", 22);
    } else if (!length) {
        sys_write("directory is empty\n", 19);
    } else {
        sys_write(file_list, (unsigned)length);
        sys_write("\n", 1);
    }
    sys_exit();
}

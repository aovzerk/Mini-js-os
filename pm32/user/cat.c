#include <myos/api.h>
#include <myos/userlib.h>

#define MAX_FILE_SIZE 4096

static char file_data[MAX_FILE_SIZE];

static int make_fat_name(const char *source, char destination[11]);
static void cat_main(const char *arguments);

void _start(const char *arguments)
{
    cat_main(arguments);
}

#pragma aux _start parm [ebx];

static int make_fat_name(const char *source, char destination[11])
{
    unsigned index;
    unsigned base_length = 0;
    unsigned extension_length = 0;
    int in_extension = 0;

    for (index = 0; index < 11; ++index) {
        destination[index] = ' ';
    }
    for (index = 0; source[index] && source[index] != ' '; ++index) {
        char character = source[index];

        if (character == '.' && !in_extension && base_length) {
            in_extension = 1;
            continue;
        }
        if (character == '.' || character == ' ') {
            return 0;
        }
        if (character >= 'a' && character <= 'z') {
            character -= 32;
        }
        if (in_extension) {
            if (extension_length >= 3) {
                return 0;
            }
            destination[8 + extension_length++] = character;
        } else {
            if (base_length >= 8) {
                return 0;
            }
            destination[base_length++] = character;
        }
    }
    return base_length != 0;
}

static void cat_main(const char *arguments)
{
    char fat_name[11];
    FileRequest request;
    int length;

    if (!arguments[0]) {
        myos_write_text("usage: cat FILE.TXT\n");
        sys_exit();
    }
    if (!make_fat_name(arguments, fat_name)) {
        myos_write_text("invalid FAT 8.3 file name\n");
        sys_exit();
    }
    request.name = fat_name;
    request.destination = file_data;
    request.capacity = sizeof(file_data);
    length = sys_read_file(&request);
    if (length < 0) {
        myos_write_text("file not found or too large\n");
        sys_exit();
    }
    if (length > 0) {
        myos_write_buffer(file_data, (unsigned)length);
    }
    if (!length || file_data[length - 1] != '\n') {
        sys_write("\n", 1);
    }
    sys_exit();
}

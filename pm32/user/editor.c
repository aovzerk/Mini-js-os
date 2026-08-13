#include <myos/api.h>
#include <myos/gui.h>
#include <myos/userlib.h>

#define MAX_TEXT 4096

static char text[MAX_TEXT];
static char fat_name[11];
static char file_list[256];

static void editor_main(void);
static int make_fat_name(const char *source, char destination[11]);

/* Flat programs always begin at the first byte of the binary image. */
void _start(void)
{
    editor_main();
}

static int make_fat_name(const char *source, char destination[11])
{
    unsigned i;
    unsigned base_length = 0;
    unsigned extension_length = 0;
    int in_extension = 0;

    for (i = 0; i < 11; ++i) {
        destination[i] = ' ';
    }
    for (i = 0; source[i]; ++i) {
        char ch = source[i];

        if (ch == '.' && !in_extension && base_length) {
            in_extension = 1;
            continue;
        }
        if (ch == '.' || ch == ' ') {
            return 0;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch -= 32;
        }
        if (in_extension) {
            if (extension_length >= 3) {
                return 0;
            }
            destination[8 + extension_length++] = ch;
        } else {
            if (base_length >= 8) {
                return 0;
            }
            destination[base_length++] = ch;
        }
    }
    return base_length != 0;
}

static void editor_main(void)
{
    char filename[13];
    unsigned filename_length = 0;
    unsigned length = 0;
    int key;
    FileRequest read_request;
    WriteRequest write_request;
    int file_list_length;

    sys_gui_set_title("TEXT EDITOR");
    if (sys_gui_create_window() < 0) {
        sys_exit();
    }
    myos_write_text("MYOS TEXT EDITOR\n\nFiles:\n");
    file_list_length = sys_list_files(file_list, sizeof(file_list));
    if (file_list_length > 0) {
        sys_write(file_list, (unsigned)file_list_length);
    } else {
        myos_write_text("(no files)");
    }
    myos_write_text("\n\nOpen or create file (8.3, .TXT or .JS): ");
    for (;;) {
        key = sys_read_key();
        if (!key) {
            sys_yield();
            continue;
        }
        if (key == 13) {
            filename[filename_length] = 0;
            if (make_fat_name(filename, fat_name) &&
                ((fat_name[8] == 'T' && fat_name[9] == 'X' && fat_name[10] == 'T') ||
                 (fat_name[8] == 'J' && fat_name[9] == 'S' && fat_name[10] == ' '))) {
                break;
            }
            filename_length = 0;
            myos_write_text("Use a .TXT or .JS file name: ");
        } else if (key == 8 && filename_length) {
            --filename_length;
        } else if (key >= 32 && key < 127 && filename_length < 12) {
            filename[filename_length++] = (char)key;
        }
    }
    read_request.name = fat_name;
    read_request.destination = text;
    read_request.capacity = MAX_TEXT;
    length = (unsigned)sys_read_file(&read_request);
    if ((int)length < 0 || length > MAX_TEXT) {
        length = 0;
    }
    while (length && !text[length - 1]) {
        --length;
    }
    myos_write_text("\n--- ESC saves and closes ---\n");
    if (length) {
        myos_write_buffer(text, length);
    }
    for (;;) {
        key = sys_read_key();
        if (!key) {
            sys_yield();
            continue;
        }
        if (key == 27) {
            myos_write_text("\nSaving...\n");
            write_request.name = fat_name;
            write_request.source = text;
            write_request.size = length;
            if (sys_write_file(&write_request) < 0) {
                myos_write_text("\nDisk write error\n");
            } else {
                myos_write_text("\nSaved\n");
            }
            sys_exit();
        } else if (key == 8 && length) {
            --length;
        } else if (key == 13 && length < MAX_TEXT) {
            text[length++] = '\n';
        } else if (key >= 32 && key < 127 && length < MAX_TEXT) {
            text[length++] = (char)key;
        }
    }
}

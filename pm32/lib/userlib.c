#include <myos/api.h>
#include <myos/userlib.h>

unsigned myos_text_length(const char *text)
{
    unsigned length = 0;

    while (text[length]) {
        ++length;
    }
    return length;
}

void myos_write_text(const char *text)
{
    myos_write_buffer(text, myos_text_length(text));
}

void myos_write_buffer(const char *buffer, unsigned length)
{
    unsigned position = 0;

    while (position < length) {
        unsigned count = length - position;

        if (count > 128) {
            count = 128;
        }
        sys_write(buffer + position, count);
        position += count;
        sys_yield();
    }
}

void *myos_malloc(unsigned size)
{
    return sys_malloc(size);
}

int myos_free(void *pointer)
{
    return sys_free(pointer);
}

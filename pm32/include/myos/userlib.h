#ifndef MYOS_USERLIB_H
#define MYOS_USERLIB_H

unsigned myos_text_length(const char *text);
void myos_write_text(const char *text);
void myos_write_buffer(const char *buffer, unsigned length);
void *myos_malloc(unsigned size);
int myos_free(void *pointer);

#endif

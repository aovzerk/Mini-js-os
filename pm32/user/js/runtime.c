static void run_source(unsigned length)
{
    unsigned position;
    unsigned count = 0;
    source[length] = 0;
    if (!strip_comments(source, length)) {
        myos_write_text("JS error: unterminated block comment\n");
        return;
    }
    source_lines[count++] = source;
    for (position = 0; position < length; ++position) {
        if (source[position] == '\r') source[position] = 0;
        if (source[position] == '\n') {
            source[position] = 0;
            if (count >= MAX_LINES) {
                myos_write_text("JS error: too many source lines\n");
                return;
            }
            source_lines[count++] = source + position + 1;
        }
    }
    execute_range(0, count, 0);
}

static void js_main(const char *arguments)
{
    char fat_name[11];
    FileRequest request;
    int length;
    if (!arguments[0] || !make_fat_name(arguments, fat_name)) {
        myos_write_text("usage: js FILE.JS\n");
        return;
    }
    request.name = fat_name;
    request.destination = source;
    request.capacity = MAX_SOURCE;
    length = sys_read_file(&request);
    if (length < 0) myos_write_text("JS file not found or too large\n");
    else {
        source[length] = 0;
        run_source((unsigned)length);
    }
}

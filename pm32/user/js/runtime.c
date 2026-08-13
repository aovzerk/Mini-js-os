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
    if (execute_range(0, count, 0)) run_timers();
}

static void js_main(const char *arguments)
{
    char fat_name[11];
    FileRequest request;
    char *main_source;
    int length;
    unsigned output = 0;
    unsigned position = 0;
    if (!arguments[0] || !make_fat_name(arguments, fat_name)) {
        myos_write_text("usage: js FILE.JS\n");
        return;
    }
    source = (char *)sys_malloc(MAX_SOURCE + 1);
    main_source = (char *)sys_malloc(MAX_SOURCE + 1);
    if (!source || !main_source) {
        myos_write_text("JS error: out of memory\n");
        if (source) sys_free(source);
        if (main_source) sys_free(main_source);
        source = 0;
        return;
    }
    request.name = fat_name;
    request.destination = main_source;
    request.capacity = MAX_SOURCE;
    length = sys_read_file(&request);
    if (length < 0) myos_write_text("JS file not found or too large\n");
    else {
        main_source[length] = 0;
        while (position < (unsigned)length) {
            unsigned line_start = position;
            unsigned line_end;
            unsigned scan;
            while (position < (unsigned)length && main_source[position] != '\n')
                ++position;
            line_end = position;
            scan = line_start;
            while (scan < line_end && is_space(main_source[scan])) ++scan;
            if (scan + 9 < line_end &&
                main_source[scan] == 'r' && main_source[scan + 1] == 'e' &&
                main_source[scan + 2] == 'q' && main_source[scan + 3] == 'u' &&
                main_source[scan + 4] == 'i' && main_source[scan + 5] == 'r' &&
                main_source[scan + 6] == 'e' && main_source[scan + 7] == '(' &&
                (main_source[scan + 8] == '"' || main_source[scan + 8] == '\'')) {
                char module_name[MAX_NAME];
                char module_fat[11];
                char quote = main_source[scan + 8];
                unsigned name_length = 0;
                unsigned name_position = scan + 9;
                while (name_position < line_end &&
                       main_source[name_position] != quote &&
                       name_length + 1 < MAX_NAME)
                    module_name[name_length++] = main_source[name_position++];
                module_name[name_length] = 0;
                if (!text_equal(module_name, "myos") &&
                    make_fat_name(module_name, module_fat)) {
                    FileRequest module_request;
                    int module_length;
                    module_request.name = module_fat;
                    module_request.destination = source + output;
                    module_request.capacity = MAX_SOURCE - output;
                    module_length = sys_read_file(&module_request);
                    if (module_length < 0) {
                        myos_write_text("JS module not found: ");
                        myos_write_text(module_name);
                        myos_write_text("\n");
                        length = -1;
                        break;
                    }
                    output += (unsigned)module_length;
                    if (output < MAX_SOURCE) source[output++] = '\n';
                }
            }
            if (position < (unsigned)length) ++position;
        }
        if (length >= 0 && output + (unsigned)length <= MAX_SOURCE) {
            unsigned i;
            for (i = 0; i < (unsigned)length; ++i)
                source[output + i] = main_source[i];
            output += (unsigned)length;
            source[output] = 0;
            run_source(output);
        } else if (length >= 0) {
            myos_write_text("JS source and modules are too large\n");
        }
    }
    sys_free(main_source);
    sys_free(source);
    source = 0;
}

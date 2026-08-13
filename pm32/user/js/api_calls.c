static int execute_myos_call(void)
{
    char method[24];
    char first[MAX_LINE];
    char second[MAX_LINE];
    long a = 0;
    long b = 0;
    int result = 0;
    int length;
    FileRequest read_request;
    WriteRequest write_request;
    const char *write_source;
    Variable *write_variable;
    call_type = VALUE_NUMBER;
    call_number = 0;
    call_text = 0;
    if (!accept("myos.")) return 0;
    if (!myos_loaded) {
        error_text = "require(\"myos\") must be called first";
        return 1;
    }
    if (!read_name(method) || !accept("(")) {
        error_text = "invalid myos API call";
        return 1;
    }
    if (text_equal(method, "readFile")) {
        if (!read_string(first, sizeof(first)) || !make_fat_name(first, api_name))
            error_text = "readFile expects a FAT 8.3 name";
        else {
            call_text = (char *)sys_malloc(MAX_SOURCE + 1);
            if (!call_text) {
                error_text = "out of memory";
                return 1;
            }
            read_request.name = api_name;
            read_request.destination = call_text;
            read_request.capacity = MAX_SOURCE;
            result = sys_read_file(&read_request);
            if (result >= 0) {
                length = result;
                call_text[length] = 0;
                call_type = VALUE_STRING;
            } else {
                sys_free(call_text);
                call_text = 0;
            }
        }
    } else if (text_equal(method, "writeFile")) {
        if (!read_string(first, sizeof(first)) || !accept(",") ||
            !make_fat_name(first, api_name)) {
            error_text = "writeFile expects name and string";
        } else {
            write_source = second;
            if (!read_string(second, sizeof(second))) {
                if (!read_name(second) ||
                    (write_variable = find_variable(second, 0)) == 0 ||
                    write_variable->type != VALUE_STRING)
                    error_text = "writeFile expects a string value";
                else write_source = write_variable->text;
            }
        }
        if (!error_text) {
            write_request.name = api_name;
            write_request.source = write_source;
            write_request.size = myos_text_length(write_source);
            result = sys_write_file(&write_request);
        }
    } else if (text_equal(method, "listFiles")) {
        call_text = (char *)sys_malloc(512);
        if (!call_text) {
            error_text = "out of memory";
            return 1;
        }
        result = sys_list_files(call_text, 511);
        if (result >= 0) {
            length = result;
            call_text[length] = 0;
            call_type = VALUE_STRING;
        } else {
            sys_free(call_text);
            call_text = 0;
        }
    } else if (text_equal(method, "listProcesses")) {
        result = sys_list_processes(process_records, MAX_PROCESS_RECORDS);
        if (result >= 0) call_type = VALUE_PROCESS_ARRAY;
    } else if (text_equal(method, "clear")) {
        sys_write("\f", 1);
    } else if (text_equal(method, "exec") || text_equal(method, "run") ||
               text_equal(method, "spawn") || text_equal(method, "spawnGui")) {
        if (!read_string(first, sizeof(first))) error_text = "command string expected";
        else if (text_equal(method, "exec")) result = sys_exec(first);
        else if (text_equal(method, "run")) result = sys_run(first);
        else if (text_equal(method, "spawn")) result = sys_spawn(first);
        else result = sys_spawn_gui(first);
    } else if (text_equal(method, "getPid")) result = sys_get_pid();
    else if (text_equal(method, "getAppType")) result = sys_get_app_type();
    else if (text_equal(method, "readKey")) result = sys_read_key();
    else if (text_equal(method, "yield")) sys_yield();
    else if (text_equal(method, "idle")) { for (;;) sys_yield(); }
    else if (text_equal(method, "createWindow")) {
        result = sys_gui_create_window();
        if (result >= 0) call_type = VALUE_WINDOW;
    }
    else if (text_equal(method, "nextWindow")) result = sys_gui_next_window();
    else if (text_equal(method, "kill")) { a = parse_expression(); sys_kill((unsigned)a); }
    else if (text_equal(method, "sendKey")) {
        a = parse_expression();
        if (!accept(",")) error_text = "sendKey expects pid and key";
        else { b = parse_expression(); sys_send_key((unsigned)a, (unsigned)b); }
    } else if (text_equal(method, "terminalRead")) {
        a = parse_expression();
        call_text = (char *)sys_malloc(257);
        if (!call_text) {
            error_text = "out of memory";
            return 1;
        }
        result = sys_terminal_read((unsigned)a, call_text);
        if (result >= 0) {
            length = result;
            call_text[length] = 0;
            call_type = VALUE_STRING;
        } else {
            sys_free(call_text);
            call_text = 0;
        }
    } else if (text_equal(method, "poweroff")) sys_poweroff();
    else if (text_equal(method, "exit")) sys_exit();
    else error_text = "unknown myos API function";
    if (!error_text && !accept(")")) error_text = "expected ')'";
    call_number = result;
    return 1;
}

static int execute_window_call(void)
{
    const char *saved = cursor;
    char object_name[MAX_NAME];
    char method[MAX_NAME];
    char literal[MAX_LINE];
    Variable *object;
    Variable *argument;
    if (!read_name(object_name)) { cursor = saved; return 0; }
    object = find_variable(object_name, 0);
    if (!object || object->type != VALUE_WINDOW || !accept(".") ||
        !read_name(method) || !accept("(")) {
        cursor = saved;
        return 0;
    }
    if (text_equal(method, "clear")) {
        sys_write("\f", 1);
    } else if (text_equal(method, "beginUpdate")) {
        sys_write("\x0E", 1);
    } else if (text_equal(method, "endUpdate")) {
        sys_write("\x0F", 1);
    } else if (text_equal(method, "setTitle")) {
        if (!read_string(literal, sizeof(literal)))
            error_text = "window.setTitle expects text";
        else if (sys_gui_set_title(literal) < 0)
            error_text = "cannot set window title";
    } else if (text_equal(method, "write")) {
        const char *argument_saved = cursor;
        if (read_string(literal, sizeof(literal))) myos_write_text(literal);
        else {
            cursor = argument_saved;
            if (!read_name(literal)) error_text = "window.write expects text";
            else {
                argument = find_variable(literal, 0);
                if (!argument || argument->type != VALUE_STRING)
                    error_text = "window.write expects a string";
                else myos_write_text(argument->text);
            }
        }
    } else if (text_equal(method, "writeCell")) {
        const char *argument_saved = cursor;
        const char *cell_text = 0;
        long cell_number = 0;
        long width;
        unsigned used;
        char array_name[MAX_NAME];
        char field[MAX_NAME];
        Variable *array;

        if (read_string(literal, sizeof(literal))) {
            cell_text = literal;
        } else {
            cursor = argument_saved;
            if (read_name(array_name) &&
                (array = find_variable(array_name, 0)) != 0 &&
                array->type == VALUE_PROCESS_ARRAY && accept("[")) {
                long index = parse_expression();
                if (!accept("]") || !accept(".") || !read_name(field) ||
                    index < 0 || index >= array->value) {
                    error_text = "invalid process field";
                } else {
                    process_property(&process_records[index], field,
                                     &cell_number, &cell_text);
                }
            } else {
                cursor = argument_saved;
                cell_number = parse_expression();
            }
        }
        if (!error_text && !accept(","))
            error_text = "window.writeCell expects value and width";
        width = error_text ? 0 : parse_expression();
        if (!error_text && width < 0) error_text = "invalid cell width";
        if (!error_text) {
            if (cell_text) {
                used = myos_text_length(cell_text);
                myos_write_text(cell_text);
            } else {
                used = number_width(cell_number);
                write_number(cell_number);
            }
            write_padding(used, (unsigned)width);
        }
    } else if (text_equal(method, "wait")) {
        for (;;) sys_yield();
    } else error_text = "unknown window method";
    if (!error_text && !accept(")")) error_text = "expected ')'";
    return 1;
}

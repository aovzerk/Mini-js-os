static int execute_line(char *line)
{
    char name[MAX_NAME];
    const char *saved;
    Variable *variable;
    long value;
    cursor = line;
    error_text = 0;
    skip_space();
    if (!*cursor || (cursor[0] == '/' && cursor[1] == '/')) return 1;

    if (accept("return")) {
        if (is_space(*cursor)) function_return_value = parse_expression();
        else function_return_value = 0;
        function_returning = 1;
    }

    saved = cursor;
    if (function_returning) {
        /* Return is handled above. */
    } else if (execute_object_assignment()) {
        /* Mutable object property. */
    } else if (execute_screen_call()) {
        /* Native compositor command. */
    } else if (execute_timer_call(&value)) {
        /* Timer registration. */
    } else if (execute_function_call(&value)) {
        /* User function call. */
    } else if (accept("require")) {
        if (!accept("(") || !read_string(name, sizeof(name)) || !accept(")"))
            error_text = "require expects a module name";
        else if (text_equal(name, "myos")) myos_loaded = 1;
        /* Local .js modules were merged before parsing. */
    } else if ((cursor = saved, execute_window_call())) {
        /* Window object method. */
    } else if ((cursor = saved, execute_myos_call())) {
        /* Built-in kernel API call. */
        discard_call_result();
    } else if ((cursor = saved, accept("let") ||
                (cursor = saved, accept("var")) ||
                (cursor = saved, accept("const"))) && is_space(*cursor)) {
        if (!read_name(name)) error_text = "expected variable name";
        variable = error_text ? 0 : find_variable(name, 1);
        if (!variable) error_text = "variable table full";
        if (!error_text && accept("=")) {
            const char *initializer = cursor;
            if (accept("require")) {
                if (!accept("(") || !read_string(line_buffer, sizeof(line_buffer)) ||
                    !text_equal(line_buffer, "myos") || !accept(")"))
                    error_text = "only require(\"myos\") is available";
                else {
                    myos_loaded = 1;
                    variable->type = VALUE_MODULE;
                }
            } else {
                cursor = initializer;
                if (read_string(line_buffer, sizeof(line_buffer))) {
                    release_variable_value(variable);
                    variable->type = VALUE_STRING;
                    if (!copy_heap_text(&variable->text, line_buffer))
                        error_text = "out of memory";
                } else if ((cursor = initializer, parse_object_literal(variable))) {
                    /* Object literal. */
                } else if (parse_array_literal(variable)) {
                    /* Array literal. */
                } else if (execute_myos_call()) {
                    store_call_result(variable);
                } else {
                    cursor = initializer;
                    release_variable_value(variable);
                    variable->type = VALUE_NUMBER;
                    variable->value = parse_expression();
                }
            }
        }
    } else {
        cursor = saved;
        if (accept("console.log")) {
            if (!accept("(")) error_text = "expected '('";
            if (!error_text) print_value();
            if (!error_text && !accept(")")) error_text = "expected ')'";
            if (!error_text) sys_write("\n", 1);
        } else {
            cursor = saved;
            if (read_name(name)) {
                const char *after_name = cursor;
                variable = find_variable(name, 0);
                skip_space();
                if (cursor[0] == '+' && cursor[1] == '+') {
                    cursor += 2;
                    if (!variable) error_text = "unknown variable";
                    else ++variable->value;
                } else if (cursor[0] == '-' && cursor[1] == '-') {
                    cursor += 2;
                    if (!variable) error_text = "unknown variable";
                    else --variable->value;
                } else if (cursor[0] == '=' && cursor[1] != '=') {
                    ++cursor;
                    if (!variable) variable = find_variable(name, 1);
                    saved = cursor;
                    if (!variable) {
                        error_text = "variable table full";
                    } else if (read_string(line_buffer, sizeof(line_buffer))) {
                        release_variable_value(variable);
                        variable->type = VALUE_STRING;
                        if (!copy_heap_text(&variable->text, line_buffer))
                            error_text = "out of memory";
                    } else if ((cursor = saved, parse_object_literal(variable))) {
                        /* Object literal. */
                    } else if (parse_array_literal(variable)) {
                        /* Array literal. */
                    } else if (execute_myos_call()) {
                        store_call_result(variable);
                    } else {
                        cursor = saved;
                        value = parse_expression();
                        if (variable) {
                            release_variable_value(variable);
                            variable->type = VALUE_NUMBER;
                            variable->value = value;
                        }
                    }
                } else {
                    cursor = saved;
                    value = parse_expression();
                    if (!error_text) { write_number(value); sys_write("\n", 1); }
                    (void)after_name;
                }
            } else {
                value = parse_expression();
                if (!error_text) { write_number(value); sys_write("\n", 1); }
            }
        }
    }
    skip_space();
    if (*cursor == ';') { ++cursor; skip_space(); }
    if (!error_text && *cursor && !(cursor[0] == '/' && cursor[1] == '/')) error_text = "unexpected token";
    if (error_text) {
        myos_write_text("JS error: ");
        myos_write_text(error_text);
        myos_write_text("\n");
        return 0;
    }
    return 1;
}

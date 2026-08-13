static int find_function(const char *name)
{
    unsigned i;
    for (i = 0; i < MAX_FUNCTIONS; ++i)
        if (functions[i].used && text_equal(functions[i].name, name)) return (int)i;
    return -1;
}

static void copy_name(char *destination, const char *source_text)
{
    unsigned i = 0;
    while (source_text[i] && i + 1 < MAX_NAME) {
        destination[i] = source_text[i];
        ++i;
    }
    destination[i] = 0;
}

static int parse_function_header(char *line, unsigned first, unsigned end)
{
    const char *saved_cursor = cursor;
    char name[MAX_NAME];
    char parameter[MAX_NAME];
    char parsed_parameters[MAX_PARAMETERS][MAX_NAME];
    unsigned slot;
    unsigned count = 0;
    int close;
    int arrow = 0;

    cursor = line;
    if (accept("function")) {
        if (!read_name(name)) { cursor = saved_cursor; return -1; }
    } else if (accept("const") || (cursor = line, accept("let")) ||
               (cursor = line, accept("var"))) {
        if (!read_name(name) || !accept("=")) { cursor = saved_cursor; return -1; }
        arrow = 1;
    } else { cursor = saved_cursor; return -1; }
    if (!accept("(")) { cursor = saved_cursor; return -1; }
    if (!accept(")")) {
        for (;;) {
            if (count >= MAX_PARAMETERS || !read_name(parameter)) {
                error_text = "invalid function parameters";
                cursor = saved_cursor;
                return -2;
            }
            copy_name(parsed_parameters[count], parameter);
            ++count;
            if (accept(")")) break;
            if (!accept(",")) {
                error_text = "expected ',' or ')'";
                cursor = saved_cursor;
                return -2;
            }
        }
    }
    if (arrow && !accept("=>")) { cursor = saved_cursor; return -1; }
    if (!accept("{") || (close = find_block_end(first, end)) < 0) {
        error_text = "function body expected";
        cursor = saved_cursor;
        return -2;
    }
    for (slot = 0; slot < MAX_FUNCTIONS && functions[slot].used; ++slot) {}
    if (slot == MAX_FUNCTIONS || find_function(name) >= 0) {
        error_text = "function table full or duplicate";
        cursor = saved_cursor;
        return -2;
    }
    functions[slot].used = 1;
    copy_name(functions[slot].name, name);
    functions[slot].parameter_count = count;
    while (count--) copy_name(functions[slot].parameters[count],
                              parsed_parameters[count]);
    functions[slot].first_line = first + 1;
    functions[slot].end_line = (unsigned)close;
    cursor = saved_cursor;
    return close;
}

static int call_function(unsigned index, JsValue *arguments, unsigned count,
                         long *result)
{
    JsFunction *function;
    Variable *parameters[MAX_PARAMETERS];
    unsigned i;
    int previous_returning = function_returning;
    long previous_value = function_return_value;
    const char *return_cursor = cursor;

    if (index >= MAX_FUNCTIONS || !functions[index].used) return 0;
    function = &functions[index];
    if (count != function->parameter_count) {
        error_text = "wrong argument count";
        return 0;
    }
    ++current_scope;
    for (i = 0; i < count; ++i) {
        parameters[i] = find_variable(function->parameters[i], 1);
        if (!parameters[i]) { error_text = "variable table full"; break; }
        move_value_to_variable(&arguments[i], parameters[i]);
    }
    function_returning = 0;
    function_return_value = 0;
    if (!error_text && execute_range(function->first_line, function->end_line, 1))
        *result = function_return_value;
    release_scope(current_scope);
    --current_scope;
    function_returning = previous_returning;
    function_return_value = previous_value;
    cursor = return_cursor;
    return !error_text;
}

static int parse_argument_value(JsValue *value)
{
    const char *saved = cursor;
    char name[MAX_NAME];
    Variable *variable;
    if (read_name(name)) {
        variable = find_variable(name, 0);
        skip_space();
        if (variable && variable->type == VALUE_ARRAY && accept("[")) {
            long index = parse_expression();
            if (!accept("]") || index < 0 ||
                index >= (long)variable->array->length) {
                error_text = "array index out of range";
                return 0;
            }
            skip_space();
            if (*cursor == ',' || *cursor == ')') {
                if (!clone_value(&variable->array->items[index], value))
                    error_text = "out of memory";
                return !error_text;
            }
        }
        if (variable && (*cursor == ',' || *cursor == ')')) {
            if (!clone_variable_value(variable, value)) error_text = "out of memory";
            return !error_text;
        }
    }
    cursor = saved;
    return parse_js_value(value);
}

static int execute_function_call(long *result)
{
    const char *saved = cursor;
    char name[MAX_NAME];
    JsValue arguments[MAX_PARAMETERS];
    unsigned count = 0;
    int index;
    unsigned i;
    for (i = 0; i < MAX_PARAMETERS; ++i) clear_value(&arguments[i]);
    if (!read_name(name) || (index = find_function(name)) < 0 || !accept("(")) {
        cursor = saved;
        return 0;
    }
    if (!accept(")")) {
        for (;;) {
            if (count >= MAX_PARAMETERS) {
                error_text = "too many arguments";
                return 1;
            }
            if (!parse_argument_value(&arguments[count])) return 1;
            ++count;
            if (accept(")")) break;
            if (!accept(",")) { error_text = "expected ',' or ')'"; return 1; }
        }
    }
    call_function((unsigned)index, arguments, count, result);
    for (i = 0; i < count; ++i) release_value(&arguments[i]);
    return 1;
}

static int execute_timer_call(long *result)
{
    const char *saved = cursor;
    char method[MAX_NAME];
    char callback[MAX_NAME];
    long delay;
    int index;
    int repeat;
    if (!read_name(method) ||
        (!text_equal(method, "setTimeout") &&
         !text_equal(method, "setInterval") &&
         !text_equal(method, "clearTimeout") &&
         !text_equal(method, "clearInterval"))) {
        cursor = saved;
        return 0;
    }
    if (text_equal(method, "clearTimeout") || text_equal(method, "clearInterval")) {
        long id;
        if (!accept("(")) {
            error_text = "clear timer expects an id";
            return 1;
        }
        id = parse_expression();
        if (!error_text && !accept(")")) error_text = "expected ')'";
        if (!error_text && id > 0 && id <= MAX_TIMERS)
            timers[id - 1].used = 0;
        *result = 0;
        return 1;
    }
    repeat = text_equal(method, "setInterval");
    if (!accept("(") || !read_name(callback) ||
        (index = find_function(callback)) < 0 || !accept(",")) {
        error_text = "timer expects a function and delay";
        return 1;
    }
    delay = parse_expression();
    if (!error_text && (!accept(")") || delay < 0))
        error_text = "invalid timer delay";
    if (!error_text) {
        int id = schedule_timer((unsigned)index, (u32)delay, repeat);
        if (id < 0) error_text = "timer table full";
        else *result = id;
    }
    return 1;
}

static int schedule_timer(unsigned function_index, u32 delay, int repeat)
{
    unsigned i;
    for (i = 0; i < MAX_TIMERS && timers[i].used; ++i) {}
    if (i == MAX_TIMERS) return -1;
    timers[i].used = 1;
    timers[i].repeat = repeat;
    timers[i].function_index = function_index;
    timers[i].delay = delay;
    timers[i].due = sys_millis() + delay;
    return (int)i + 1;
}

static void run_timers(void)
{
    for (;;) {
        unsigned i;
        int active = 0;
        u32 next_due = 0;
        u32 now = sys_millis();
        for (i = 0; i < MAX_TIMERS; ++i) {
            if (timers[i].used) {
                long ignored;
                active = 1;
                if ((long)(now - timers[i].due) >= 0) {
                    if (timers[i].repeat) timers[i].due = now + timers[i].delay;
                    else timers[i].used = 0;
                    if (!call_function(timers[i].function_index, 0, 0, &ignored))
                        return;
                }
                if (timers[i].used &&
                    (!next_due ||
                     (long)(timers[i].due - next_due) < 0))
                    next_due = timers[i].due;
            }
        }
        if (!active) return;
        if (next_due && (long)(next_due - sys_millis()) > 0)
            sys_wait_until(next_due);
    }
}

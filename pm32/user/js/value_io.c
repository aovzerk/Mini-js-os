static void write_number(long value)
{
    char buffer[16];
    unsigned length = 0;
    unsigned start;
    unsigned i;
    unsigned long magnitude;
    if (value < 0) {
        buffer[length++] = '-';
        magnitude = (unsigned long)(-(value + 1)) + 1;
    } else magnitude = (unsigned long)value;
    start = length;
    do {
        buffer[length++] = (char)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude && length < sizeof(buffer));
    for (i = start; i < start + (length - start) / 2; ++i) {
        char temporary = buffer[i];
        buffer[i] = buffer[length - 1 - (i - start)];
        buffer[length - 1 - (i - start)] = temporary;
    }
    sys_write(buffer, length);
}

static unsigned number_width(long value)
{
    unsigned width = value < 0 ? 1 : 0;
    unsigned long magnitude = value < 0
        ? (unsigned long)(-(value + 1)) + 1
        : (unsigned long)value;
    do {
        ++width;
        magnitude /= 10;
    } while (magnitude);
    return width;
}

static void write_padding(unsigned used, unsigned width)
{
    static const char spaces[] = "                                ";
    while (used < width) {
        unsigned count = width - used;
        if (count > sizeof(spaces) - 1) count = sizeof(spaces) - 1;
        sys_write(spaces, count);
        used += count;
    }
}

static void write_quoted_text(const char *text)
{
    char quote = '"';
    sys_write(&quote, 1);
    while (*text) {
        if (*text == '"' || *text == '\\') {
            char slash = '\\';
            sys_write(&slash, 1);
            sys_write(text, 1);
        } else if (*text == '\n') {
            sys_write("\\n", 2);
        } else if (*text == '\t') {
            sys_write("\\t", 2);
        } else {
            sys_write(text, 1);
        }
        ++text;
    }
    sys_write(&quote, 1);
}

static void write_object(Object *object)
{
    ObjectProperty *property = object ? object->first : 0;
    sys_write("{", 1);
    while (property) {
        myos_write_text(property->name);
        sys_write(": ", 2);
        if (property->type == VALUE_STRING)
            write_quoted_text(property->text);
        else
            write_number(property->number);
        property = property->next;
        if (property) sys_write(", ", 2);
    }
    sys_write("}", 1);
}

static int print_value(void)
{
    char quote;
    const char *saved;
    char name[MAX_NAME];
    Variable *variable;
    ObjectProperty *property;
    skip_space();
    if (*cursor == '\'' || *cursor == '"') {
        quote = *cursor++;
        while (*cursor && *cursor != quote) {
            char ch = *cursor++;
            if (ch == '\\') {
                ch = *cursor++;
                if (ch == 'n') ch = '\n';
                else if (ch == 't') ch = '\t';
            }
            sys_write(&ch, 1);
        }
        if (*cursor != quote) {
            error_text = "unterminated string";
            return 0;
        }
        ++cursor;
    } else {
        saved = cursor;
        {
            long function_value;
            if (execute_function_call(&function_value)) {
                if (!error_text) write_number(function_value);
                return !error_text;
            }
            cursor = saved;
        }
        if (read_name(name)) {
            variable = find_variable(name, 0);
            skip_space();
            if (variable && variable->type == VALUE_STRING && *cursor == ')') {
                myos_write_text(variable->text);
                return 1;
            }
            if (variable && variable->type == VALUE_OBJECT && *cursor == ')') {
                write_object(variable->object);
                return 1;
            }
            if (variable && variable->type == VALUE_OBJECT && accept(".") &&
                read_name(name) && *cursor == ')') {
                property = find_object_property(variable->object, name);
                if (!property) {
                    error_text = "unknown object property";
                    return 0;
                }
                if (property->type == VALUE_STRING)
                    myos_write_text(property->text);
                else write_number(property->number);
                return 1;
            }
        }
        cursor = saved;
        write_number(parse_expression());
    }
    return !error_text;
}

static int read_string(char *destination, unsigned capacity)
{
    char quote;
    unsigned length = 0;
    skip_space();
    if (*cursor != '\'' && *cursor != '"') return 0;
    quote = *cursor++;
    while (*cursor && *cursor != quote) {
        char ch = *cursor++;
        if (ch == '\\' && *cursor) {
            ch = *cursor++;
            if (ch == 'n') ch = '\n';
            else if (ch == 't') ch = '\t';
        }
        if (length + 1 < capacity) destination[length++] = ch;
    }
    if (*cursor != quote) return 0;
    ++cursor;
    destination[length] = 0;
    return 1;
}

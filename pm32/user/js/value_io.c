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

static int print_value(void)
{
    char quote;
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
    } else write_number(parse_expression());
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

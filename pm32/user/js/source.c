static int make_fat_name(const char *source_name, char destination[11])
{
    unsigned i;
    unsigned base = 0;
    unsigned extension = 0;
    int dot = 0;
    for (i = 0; i < 11; ++i) destination[i] = ' ';
    for (i = 0; source_name[i] && source_name[i] != ' '; ++i) {
        char ch = source_name[i];
        if (ch == '.' && !dot && base) { dot = 1; continue; }
        if (ch == '.' || !is_name_char(ch)) return 0;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        if (dot) {
            if (extension >= 3) return 0;
            destination[8 + extension++] = ch;
        } else {
            if (base >= 8) return 0;
            destination[base++] = ch;
        }
    }
    return base && extension;
}

#include "comments.c"

static char *trim_line(char *line)
{
    while (is_space(*line)) ++line;
    return line;
}

static int begins_word(const char *line, const char *word)
{
    unsigned i = 0;
    line = trim_line((char *)line);
    while (word[i] && line[i] == word[i]) ++i;
    return !word[i] && (is_space(line[i]) || line[i] == '(');
}

static int brace_change(const char *line)
{
    int change = 0;
    char quote = 0;
    while (*line) {
        if (quote) {
            if (*line == '\\' && line[1]) ++line;
            else if (*line == quote) quote = 0;
        } else if (*line == '\'' || *line == '"') quote = *line;
        else if (line[0] == '/' && line[1] == '/') break;
        else if (*line == '{') ++change;
        else if (*line == '}') --change;
        ++line;
    }
    return change;
}

static int find_block_end(unsigned start, unsigned end)
{
    unsigned i;
    int depth = 0;
    for (i = start; i < end; ++i) {
        depth += brace_change(source_lines[i]);
        if (depth == 0) return (int)i;
    }
    return -1;
}

static int copy_parentheses(const char *line, char *destination)
{
    const char *start = line;
    const char *finish = 0;
    unsigned length;
    while (*start && *start != '(') ++start;
    if (*start != '(') return 0;
    ++start;
    line = start;
    while (*line) {
        if (*line == ')') finish = line;
        ++line;
    }
    if (!finish || finish < start) return 0;
    length = (unsigned)(finish - start);
    if (length + 1 >= MAX_LINE) return 0;
    while (length) {
        *destination++ = *start++;
        --length;
    }
    *destination = 0;
    return 1;
}

static int evaluate_condition(char *text, long *result)
{
    cursor = text;
    error_text = 0;
    *result = parse_expression();
    skip_space();
    if (!error_text && *cursor) error_text = "unexpected token in condition";
    if (error_text) {
        myos_write_text("JS error: ");
        myos_write_text(error_text);
        myos_write_text("\n");
        return 0;
    }
    return 1;
}

static int split_for_parts(char *header)
{
    unsigned part = 0;
    unsigned length = 0;
    char quote = 0;
    while (*header) {
        char ch = *header++;
        if (quote) {
            if (ch == quote) quote = 0;
        } else if (ch == '\'' || ch == '"') quote = ch;
        else if (ch == ';') {
            if (part >= 2) return 0;
            control_parts[part][length] = 0;
            ++part;
            length = 0;
            continue;
        }
        if (length + 1 >= MAX_LINE) return 0;
        control_parts[part][length++] = ch;
    }
    control_parts[part][length] = 0;
    return part == 2;
}

static void report_line(unsigned line_number)
{
    myos_write_text("at line ");
    write_number((long)line_number);
    sys_write("\n", 1);
}

static void copy_text(char *destination, const char *source_text)
{
    unsigned i = 0;
    while (source_text[i] && i + 1 < MAX_LINE) {
        destination[i] = source_text[i];
        ++i;
    }
    destination[i] = 0;
}

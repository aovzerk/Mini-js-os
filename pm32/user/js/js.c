#include <myos/api.h>
#include <myos/gui.h>
#include <myos/userlib.h>

#define MAX_SOURCE 4096
#define MAX_LINE 256
#define MAX_VARIABLES 32
#define MAX_NAME 16
#define MAX_LINES 128
#define MAX_BLOCK_DEPTH 8
#define LOOP_LIMIT 10000UL
#define VALUE_NUMBER 0
#define VALUE_STRING 1
#define VALUE_MODULE 2
#define VALUE_WINDOW 3
#define VALUE_PROCESS_ARRAY 4
#define MAX_PROCESS_RECORDS 8

typedef struct Variable {
    char name[MAX_NAME];
    long value;
    int used;
    int type;
    char text[512];
} Variable;

static char source[MAX_SOURCE + 1];
static char api_buffer[MAX_SOURCE + 1];
static char api_name[11];
static char line_buffer[MAX_LINE];
static Variable variables[MAX_VARIABLES];
static char *source_lines[MAX_LINES];
static char control_parts[3][MAX_LINE];
static char loop_parts[MAX_BLOCK_DEPTH][3][MAX_LINE];
static const char *cursor;
static const char *error_text;
static int myos_loaded;
static int call_type;
static long call_number;
static char call_text[512];
static ProcessInfo process_records[MAX_PROCESS_RECORDS];
static const char *startup_arguments;
static Variable *find_variable(const char *name, int create);
static void js_main(const char *arguments);
static int execute_line(char *line);

void _start(const char *arguments)
{
    int graphical;
    startup_arguments = arguments;
    graphical = sys_get_app_type() == APP_GUI;
    if (graphical) sys_gui_create_window();
    js_main(startup_arguments);
    if (graphical) {
        for (;;) sys_yield();
    }
    if (sys_get_pid() == 0) sys_exec("shell");
    sys_exit();
    for (;;) sys_yield();
}

#pragma aux _start parm [ebx];

static void set_result(long value)
{
    Variable *variable = find_variable("result", 1);
    if (variable) variable->value = value;
}

static int is_space(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static int is_name_start(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static int is_name_char(char ch)
{
    return is_name_start(ch) || (ch >= '0' && ch <= '9');
}

static void skip_space(void)
{
    while (is_space(*cursor)) ++cursor;
}

static int text_equal(const char *left, const char *right)
{
    unsigned i = 0;
    while (left[i] && right[i] && left[i] == right[i]) ++i;
    return left[i] == right[i];
}

static int accept(const char *token)
{
    const char *saved = cursor;
    unsigned i = 0;
    skip_space();
    while (token[i] && cursor[i] == token[i]) ++i;
    if (token[i]) {
        cursor = saved;
        return 0;
    }
    cursor += i;
    return 1;
}

static int read_name(char name[MAX_NAME])
{
    unsigned length = 0;
    skip_space();
    if (!is_name_start(*cursor)) return 0;
    while (is_name_char(*cursor)) {
        if (length + 1 < MAX_NAME) name[length++] = *cursor;
        ++cursor;
    }
    name[length] = 0;
    return 1;
}

static Variable *find_variable(const char *name, int create)
{
    unsigned i;
    Variable *empty = 0;
    for (i = 0; i < MAX_VARIABLES; ++i) {
        if (variables[i].used && text_equal(variables[i].name, name)) return &variables[i];
        if (!variables[i].used && !empty) empty = &variables[i];
    }
    if (!create || !empty) return 0;
    empty->used = 1;
    empty->value = 0;
    empty->type = VALUE_NUMBER;
    empty->text[0] = 0;
    for (i = 0; i + 1 < MAX_NAME && name[i]; ++i) empty->name[i] = name[i];
    empty->name[i] = 0;
    return empty;
}

static long parse_expression(void);
static int make_fat_name(const char *source_name, char destination[11]);

#include "expressions.c"
#include "value_io.c"
#include "api_calls.c"
#include "statements.c"
#include "source.c"
#include "control_flow.c"
#include "runtime.c"

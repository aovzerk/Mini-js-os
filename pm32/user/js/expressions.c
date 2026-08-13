#include "process_values.c"

static long parse_primary(void)
{
    long value = 0;
    char name[MAX_NAME];
    Variable *variable;
    skip_space();
    {
        const char *saved = cursor;
        if (execute_timer_call(&value)) return value;
        cursor = saved;
        if (execute_function_call(&value)) return value;
        cursor = saved;
    }
    if (accept("(")) {
        value = parse_expression();
        if (!accept(")") && !error_text) error_text = "expected ')'";
        return value;
    }
    if (*cursor >= '0' && *cursor <= '9') {
        while (*cursor >= '0' && *cursor <= '9') {
            value = value * 10 + (*cursor++ - '0');
        }
        return value;
    }
    if (read_name(name)) {
        if (text_equal(name, "true")) return 1;
        if (text_equal(name, "false")) return 0;
        variable = find_variable(name, 0);
        if (!variable) error_text = "unknown variable";
        else if (variable->type == VALUE_OBJECT) {
            ObjectProperty *property;
            if (!accept(".") || !read_name(name)) {
                error_text = "object property expected";
                return 0;
            }
            property = find_object_property(variable->object, name);
            if (!property) {
                error_text = "unknown object property";
                return 0;
            }
            if (property->type == VALUE_STRING) {
                error_text = "string used as number";
                return 0;
            }
            return property->number;
        }
        else if (variable->type == VALUE_PROCESS_ARRAY) {
            if (accept(".")) {
                if (!read_name(name) || !text_equal(name, "length"))
                    error_text = "process array only has length";
                return variable->value;
            }
            if (accept("[")) {
                long index = parse_expression();
                const char *property_text;
                if (!accept("]") || !accept(".") || !read_name(name)) {
                    error_text = "expected process property";
                    return 0;
                }
                if (index < 0 || index >= variable->value) {
                    error_text = "process index out of range";
                    return 0;
                }
                if (!process_property(&process_records[index], name,
                                      &value, &property_text)) return 0;
                if (property_text) error_text = "string used as number";
                return value;
            }
            error_text = "process array requires index";
            return 0;
        }
        return variable ? variable->value : 0;
    }
    if (!error_text) error_text = "expected expression";
    return 0;
}

static long parse_unary(void)
{
    skip_space();
    if (accept("!")) return !parse_unary();
    if (accept("-")) return -parse_unary();
    if (accept("+")) return parse_unary();
    return parse_primary();
}

static long parse_product(void)
{
    long value = parse_unary();
    for (;;) {
        if (accept("*")) value *= parse_unary();
        else if (accept("/")) {
            long right = parse_unary();
            if (!right) error_text = "division by zero";
            else value /= right;
        } else if (accept("%")) {
            long right = parse_unary();
            if (!right) error_text = "division by zero";
            else value %= right;
        } else break;
    }
    return value;
}

static long parse_sum(void)
{
    long value = parse_product();
    for (;;) {
        if (accept("+")) value += parse_product();
        else if (accept("-")) value -= parse_product();
        else break;
    }
    return value;
}

static long parse_compare(void)
{
    long value = parse_sum();
    for (;;) {
        if (accept("<=")) value = value <= parse_sum();
        else if (accept(">=")) value = value >= parse_sum();
        else if (accept("<")) value = value < parse_sum();
        else if (accept(">")) value = value > parse_sum();
        else break;
    }
    return value;
}

static long parse_expression(void)
{
    long value = parse_compare();
    for (;;) {
        if (accept("==")) value = value == parse_compare();
        else if (accept("!=")) value = value != parse_compare();
        else if (accept("&&")) {
            long right = parse_compare();
            value = value && right;
        } else if (accept("||")) {
            long right = parse_compare();
            value = value || right;
        }
        else break;
    }
    return value;
}

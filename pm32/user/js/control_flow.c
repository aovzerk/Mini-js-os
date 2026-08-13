static int execute_range(unsigned first, unsigned end, unsigned depth)
{
    unsigned i = first;
    if (depth >= MAX_BLOCK_DEPTH) {
        myos_write_text("JS error: blocks nested too deeply\n");
        return 0;
    }
    while (i < end) {
        char *line = trim_line(source_lines[i]);
        int close;
        long condition;
        unsigned iterations;
        int function_close;
        if (!*line || (line[0] == '/' && line[1] == '/')) { ++i; continue; }
        function_close = parse_function_header(line, i, end);
        if (function_close >= 0) {
            i = (unsigned)function_close + 1;
            continue;
        }
        if (function_close == -2) { report_line(i + 1); return 0; }
        if (begins_word(line, "if")) {
            if (!copy_parentheses(line, control_parts[0]) || brace_change(line) <= 0) {
                myos_write_text("JS error: expected if (condition) {\n");
                report_line(i + 1);
                return 0;
            }
            close = find_block_end(i, end);
            if (close < 0 || !evaluate_condition(control_parts[0], &condition)) {
                if (close < 0) myos_write_text("JS error: missing '}'\n");
                report_line(i + 1);
                return 0;
            }
            if (condition && !execute_range(i + 1, (unsigned)close, depth + 1)) return 0;
            i = (unsigned)close + 1;
            if (i < end && begins_word(source_lines[i], "else")) {
                int else_close;
                if (brace_change(source_lines[i]) <= 0 ||
                    (else_close = find_block_end(i, end)) < 0) {
                    myos_write_text("JS error: expected else {\n");
                    report_line(i + 1);
                    return 0;
                }
                if (!condition && !execute_range(i + 1, (unsigned)else_close, depth + 1)) return 0;
                i = (unsigned)else_close + 1;
            }
            continue;
        }
        if (begins_word(line, "while")) {
            if (!copy_parentheses(line, control_parts[0]) || brace_change(line) <= 0 ||
                (close = find_block_end(i, end)) < 0) {
                myos_write_text("JS error: expected while (condition) {\n");
                report_line(i + 1);
                return 0;
            }
            copy_text(loop_parts[depth][0], control_parts[0]);
            iterations = 0;
            for (;;) {
                if (!evaluate_condition(loop_parts[depth][0], &condition)) { report_line(i + 1); return 0; }
                if (!condition) break;
                ++iterations;
                if (!text_equal(trim_line(loop_parts[depth][0]), "true") &&
                    iterations > LOOP_LIMIT) {
                    myos_write_text("JS error: loop iteration limit reached\n");
                    report_line(i + 1);
                    return 0;
                }
                if (!execute_range(i + 1, (unsigned)close, depth + 1)) return 0;
                if (!(iterations & 63)) sys_yield();
            }
            i = (unsigned)close + 1;
            continue;
        }
        if (begins_word(line, "for")) {
            if (!copy_parentheses(line, line_buffer) || !split_for_parts(line_buffer) ||
                brace_change(line) <= 0 || (close = find_block_end(i, end)) < 0) {
                myos_write_text("JS error: expected for (init; condition; step) {\n");
                report_line(i + 1);
                return 0;
            }
            copy_text(loop_parts[depth][0], control_parts[0]);
            copy_text(loop_parts[depth][1], control_parts[1]);
            copy_text(loop_parts[depth][2], control_parts[2]);
            if (!execute_line(loop_parts[depth][0])) { report_line(i + 1); return 0; }
            iterations = 0;
            for (;;) {
                if (!evaluate_condition(loop_parts[depth][1], &condition)) { report_line(i + 1); return 0; }
                if (!condition) break;
                if (++iterations > LOOP_LIMIT) {
                    myos_write_text("JS error: loop iteration limit reached\n");
                    report_line(i + 1);
                    return 0;
                }
                if (!execute_range(i + 1, (unsigned)close, depth + 1)) return 0;
                if (!execute_line(loop_parts[depth][2])) { report_line(i + 1); return 0; }
                if (!(iterations & 63)) sys_yield();
            }
            i = (unsigned)close + 1;
            continue;
        }
        if (*line == '}') { ++i; continue; }
        if (!execute_line(line)) { report_line(i + 1); return 0; }
        if (function_returning) return 1;
        ++i;
    }
    return 1;
}

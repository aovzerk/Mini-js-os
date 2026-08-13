/* Included by js.c: comment lexer pass that preserves source line numbers. */
static int strip_comments(char *text, unsigned length)
{
    unsigned i = 0;
    int line_comment = 0;
    int block_comment = 0;
    char quote = 0;

    while (i < length) {
        char ch = text[i];

        if (line_comment) {
            if (ch == '\n' || ch == '\r') line_comment = 0;
            else text[i] = ' ';
        } else if (block_comment) {
            if (ch == '*' && i + 1 < length && text[i + 1] == '/') {
                text[i] = ' ';
                text[i + 1] = ' ';
                block_comment = 0;
                i += 2;
                continue;
            }
            if (ch != '\n' && ch != '\r') text[i] = ' ';
        } else if (quote) {
            if (ch == '\\' && i + 1 < length) {
                i += 2;
                continue;
            }
            if (ch == quote) quote = 0;
        } else if (ch == '\'' || ch == '"') {
            quote = ch;
        } else if (ch == '/' && i + 1 < length && text[i + 1] == '/') {
            text[i] = ' ';
            text[i + 1] = ' ';
            line_comment = 1;
            i += 2;
            continue;
        } else if (ch == '/' && i + 1 < length && text[i + 1] == '*') {
            text[i] = ' ';
            text[i + 1] = ' ';
            block_comment = 1;
            i += 2;
            continue;
        }
        ++i;
    }
    return !block_comment;
}

static int submit_draw(GuiDrawCommand *command)
{
    unsigned attempts = 0;
    while (sys_gui_submit_draw(command) < 0 && attempts++ < 1000) {}
    if (attempts >= 1000) {
        error_text = "GUI draw queue timeout";
        return 0;
    }
    return 1;
}

static int execute_screen_call(void)
{
    const char *saved = cursor;
    char object_name[MAX_NAME];
    char method[MAX_NAME];
    char text[MAX_LINE];
    Variable *screen;
    GuiDrawCommand command;
    unsigned i;
    if (!read_name(object_name)) { cursor = saved; return 0; }
    screen = find_variable(object_name, 0);
    if (!screen || screen->type != VALUE_SCREEN || !accept(".") ||
        !read_name(method) || !accept("(")) {
        cursor = saved;
        return 0;
    }
    command.pid = 0;
    command.target_pid = 0;
    command.layer = (u32)screen->value;
    command.x = 0;
    command.y = 0;
    command.width = 0;
    command.height = 0;
    command.color = 0;
    command.text[0] = 0;
    if (text_equal(method, "setLayer")) {
        long layer = parse_expression();
        if (layer < 0 || layer > 2)
            error_text = "screen layer must be 0, 1 or 2";
        else screen->value = layer;
        command.type = 0;
    } else if (text_equal(method, "fillRect")) {
        command.type = GUI_DRAW_FILL_RECT;
        command.x = parse_expression();
        if (!accept(",")) error_text = "fillRect expects x, y, width, height, color";
        if (!error_text) command.y = parse_expression();
        if (!error_text && !accept(",")) error_text = "fillRect expects width";
        if (!error_text) command.width = parse_expression();
        if (!error_text && !accept(",")) error_text = "fillRect expects height";
        if (!error_text) command.height = parse_expression();
        if (!error_text && !accept(",")) error_text = "fillRect expects color";
        if (!error_text) command.color = (u32)parse_expression();
    } else if (text_equal(method, "drawText")) {
        const char *source_text = 0;
        char value_name[MAX_NAME];
        command.type = GUI_DRAW_TEXT;
        command.x = parse_expression();
        if (!accept(",")) error_text = "drawText expects x, y, text";
        if (!error_text) command.y = parse_expression();
        if (!error_text && !accept(",")) error_text = "drawText expects text";
        if (!error_text && read_string(text, sizeof(text))) {
            source_text = text;
        } else if (!error_text && read_name(value_name)) {
            Variable *value = find_variable(value_name, 0);
            if (!value || value->type != VALUE_STRING)
                error_text = "drawText expects a string";
            else source_text = value->text;
        } else if (!error_text) {
            error_text = "drawText expects a string";
        }
        for (i = 0; !error_text && i < 63 && source_text[i]; ++i)
            command.text[i] = source_text[i];
        command.text[i] = 0;
    } else if (text_equal(method, "drawTerminal")) {
        command.type = GUI_DRAW_TERMINAL;
        command.target_pid = (u32)parse_expression();
        if (!accept(",")) error_text = "drawTerminal expects pid, x, y, width, height";
        if (!error_text) command.x = parse_expression();
        if (!error_text && !accept(",")) error_text = "drawTerminal expects y";
        if (!error_text) command.y = parse_expression();
        if (!error_text && !accept(",")) error_text = "drawTerminal expects width";
        if (!error_text) command.width = parse_expression();
        if (!error_text && !accept(",")) error_text = "drawTerminal expects height";
        if (!error_text) command.height = parse_expression();
    } else if (text_equal(method, "drawImage")) {
        char image_name[MAX_NAME];
        command.type = GUI_DRAW_IMAGE;
        if (!read_string(image_name, sizeof(image_name)) ||
            !make_fat_name(image_name, command.text))
            error_text = "drawImage expects a file name";
        if (!error_text && !accept(",")) error_text = "drawImage expects x, y";
        if (!error_text) command.x = parse_expression();
        if (!error_text && !accept(",")) error_text = "drawImage expects y";
        if (!error_text) command.y = parse_expression();
    } else if (text_equal(method, "drawCursor")) {
        command.type = GUI_DRAW_CURSOR;
        command.x = parse_expression();
        if (!accept(",")) error_text = "drawCursor expects x, y";
        if (!error_text) command.y = parse_expression();
    } else if (text_equal(method, "focus")) {
        command.type = 0;
        sys_gui_set_focus((unsigned)parse_expression());
    } else if (text_equal(method, "present")) {
        command.type = GUI_DRAW_PRESENT;
    } else error_text = "unknown screen method";
    if (!error_text && !accept(")")) error_text = "expected ')'";
    if (!error_text && command.type) submit_draw(&command);
    return 1;
}

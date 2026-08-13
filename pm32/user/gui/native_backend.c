#define NATIVE_DRAW_LIMIT 48
#define NATIVE_CLIENT_LIMIT 3
#define NATIVE_LAYER_COUNT 3
#define NATIVE_DRAIN_BUDGET 32
#define NATIVE_FRAME_MS 33

typedef struct NativeClient {
    int used;
    u32 pid;
    GuiDrawCommand pending[NATIVE_LAYER_COUNT][NATIVE_DRAW_LIMIT];
    GuiDrawCommand committed[NATIVE_LAYER_COUNT][NATIVE_DRAW_LIMIT];
    unsigned pending_count[NATIVE_LAYER_COUNT];
    unsigned committed_count[NATIVE_LAYER_COUNT];
    int frame_open[NATIVE_LAYER_COUNT];
} NativeClient;

static NativeClient native_clients[NATIVE_CLIENT_LIMIT];
typedef struct NativeTerminalCache {
    u32 pid;
    unsigned size;
    char text[TERMINAL_BUFFER + 1];
} NativeTerminalCache;

static NativeTerminalCache native_terminal_caches[2];
static int native_redraw_pending;
static unsigned native_last_redraw;

static NativeTerminalCache *native_terminal_cache(u32 pid)
{
    unsigned i;
    NativeTerminalCache *empty = 0;
    for (i = 0; i < 2; ++i) {
        if (native_terminal_caches[i].pid == pid)
            return &native_terminal_caches[i];
        if (!native_terminal_caches[i].pid && !empty)
            empty = &native_terminal_caches[i];
    }
    if (empty) empty->pid = pid;
    return empty;
}

static void native_terminal_echo(u32 pid, u8 key)
{
    NativeTerminalCache *cache = native_terminal_cache(pid);
    if (!cache) return;
    if (key == '\b') {
        if (cache->size) --cache->size;
    } else {
        if (key == 13) key = '\n';
        if (cache->size < TERMINAL_BUFFER)
            cache->text[cache->size++] = (char)key;
    }
}

static NativeClient *native_client(u32 pid)
{
    unsigned i;
    NativeClient *empty = 0;
    for (i = 0; i < NATIVE_CLIENT_LIMIT; ++i) {
        if (native_clients[i].used && native_clients[i].pid == pid)
            return &native_clients[i];
        if (!native_clients[i].used && !empty) empty = &native_clients[i];
    }
    if (empty) {
        empty->used = 1;
        empty->pid = pid;
    }
    return empty;
}

static void apply_native_command(const GuiDrawCommand *command)
{
    if (command->type == GUI_DRAW_FOCUS) {
        desktop_input_pid = (int)command->target_pid;
    } else if (command->type == GUI_DRAW_FILL_RECT) {
        fill((int)command->x, (int)command->y,
             (int)command->width, (int)command->height, command->color);
    } else if (command->type == GUI_DRAW_TEXT) {
        draw_text((int)command->x, (int)command->y, command->text);
    } else if (command->type == GUI_DRAW_TERMINAL) {
        NativeTerminalCache *cache =
            native_terminal_cache(command->target_pid);
        char incoming[257];
        int received;
        int length;
        int column = 0;
        int row = 0;
        int i;
        int columns = command->width / 8;
        int rows = command->height / 18;
        if (!cache) return;
        received = sys_terminal_read(command->target_pid, incoming);
        if (received > 0) {
            for (i = 0; i < received; ++i) {
                char ch = incoming[i];
                if (ch == '\f' || ch == 0x0E) cache->size = 0;
                else if (ch == '\b') { if (cache->size) --cache->size; }
                else if (ch != 0x0F) {
                    if (cache->size == TERMINAL_BUFFER) {
                        unsigned j;
                        for (j = 1; j < cache->size; ++j)
                            cache->text[j - 1] = cache->text[j];
                        --cache->size;
                    }
                    cache->text[cache->size++] = ch;
                }
            }
        }
        length = (int)cache->size;
        if (columns < 1) columns = 1;
        if (rows < 1) rows = 1;
        for (i = 0; i < length; ++i) {
            u8 ch = (u8)cache->text[i];
            if (ch == '\n') { column = 0; ++row; }
            else {
                if (row < rows)
                    draw_char((int)command->x + column * 8,
                              (int)command->y + row * 18, ch);
                if (++column >= columns) { column = 0; ++row; }
            }
            if (row >= rows) break;
        }
    } else if (command->type == GUI_DRAW_IMAGE) {
        unsigned i;
        for (i = 0; i < 11 && command->text[i] == icon_name[i]; ++i) {}
        if (i == 11 && icon_width)
            draw_icon((int)command->x, (int)command->y);
    } else if (command->type == GUI_DRAW_CURSOR) {
        cursor_draw((int)command->x, (int)command->y);
    }
}

static void replay_native_draw_layer(unsigned layer)
{
    unsigned client;
    unsigned i;
    for (client = 0; client < NATIVE_CLIENT_LIMIT; ++client) {
        NativeClient *state = &native_clients[client];
        if (!state->used) continue;
        for (i = 0; i < state->committed_count[layer]; ++i)
            apply_native_command(&state->committed[layer][i]);
    }
}

static void process_native_draw_commands(void)
{
    GuiDrawCommand command;
    unsigned drained = 0;
    unsigned now;

    /* Never drain an unbounded producer queue here.  The desktop can submit
       another cursor frame while this process is running; waiting for the
       queue to become empty made the GUI loop capable of starving input. */
    while (drained++ < NATIVE_DRAIN_BUDGET &&
           sys_gui_next_draw(&command) == 0) {
        NativeClient *state = native_client(command.pid);
        unsigned layer = command.layer >= NATIVE_LAYER_COUNT
            ? NATIVE_LAYER_COUNT - 1 : command.layer;
        if (!state) continue;
        if (command.type == GUI_DRAW_PRESENT) {
            unsigned i;
            state->committed_count[layer] = state->pending_count[layer];
            for (i = 0; i < state->pending_count[layer]; ++i)
                state->committed[layer][i] = state->pending[layer][i];
            state->pending_count[layer] = 0;
            state->frame_open[layer] = 0;
            native_redraw_pending = 1;
        } else {
            if (!state->frame_open[layer]) {
                state->pending_count[layer] = 0;
                state->frame_open[layer] = 1;
            }
            if (state->pending_count[layer] < NATIVE_DRAW_LIMIT)
                state->pending[layer][state->pending_count[layer]++] = command;
        }
    }
    /* PRESENT is a request, not an obligation to copy 3.5 MiB immediately.
       Coalesce completed frames and refresh at most once per display tick. */
    now = sys_millis();
    if (native_redraw_pending &&
        (native_last_redraw == 0 || now - native_last_redraw >= NATIVE_FRAME_MS)) {
        redraw();
        /* Measure the quiet period from the end of the expensive framebuffer
           copy.  Using the timestamp from before redraw() caused back-to-back
           frames whenever software rendering itself took over one period. */
        native_last_redraw = sys_millis();
        native_redraw_pending = 0;
    }
}

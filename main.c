#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#define FONT_SIZE 33
#define TAB_SIZE 4
#define CODEPOINT_LEN 250
#define STRING_INIT_CAP (1024*4)

// Inspired by alabaster.nvim colorscheme
// https://sr.ht/~p00f/alabaster.nvim/
#define COLOR_BG   0x0e1415ff
#define COLOR_FG   0xcececeff
#define COLOR_BLUE 0x007accff
#define COLOR_CMD  0x182325ff

#define KEY_SEMICOLON 47

#define TODO(msg) \
    do { \
        fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, msg); \
        abort(); \
    } while(0)

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} String;

void string_check_capacity(String *s)
{
    if (s->len >= s->cap)
    {
        if (s->cap == 0) s->cap = STRING_INIT_CAP;

        while (s->len >= s->cap) s->cap *= 2;

        void *buf = realloc(s->data, s->cap * sizeof(*s->data));
        assert(buf != NULL && "Failed to realloc string");

        s->data = (char*)buf;
    }
}

void string_init(String *s)
{
    string_check_capacity(s);
    memset(s->data, 0, sizeof(char)*s->cap);
    s->len = 0;
}

int string_from_file(String *s, const char *file_path)
{
    string_init(s);

    FILE *file = fopen(file_path, "rb");

    if (!file) {
        fprintf(stderr, "[ERROR] Tried to open '%s': %s\n", file_path, strerror(errno));
        return -1;
    }

    int ret = fseek(file, 0L, SEEK_END);

    if (ret == -1) {
        fprintf(stderr, "[ERROR] Tried to seek to end of '%s': %s\n", file_path, strerror(errno));
        fclose(file);
        return -1;
    }

    long int file_len = ftell(file);

    if (file_len == -1) {
        fprintf(stderr, "[ERROR] Tried to obtain position of '%s': %s\n", file_path, strerror(errno));
        fclose(file);
        return -1;
    }

    rewind(file);

    s->len = (size_t)file_len;
    string_check_capacity(s);

    size_t buffer_len = fread(s->data, sizeof(char), (size_t)file_len ,file);

    if (ferror(file)) {
        fprintf(stderr, "[ERROR] Tried to read '%s': %s\n", file_path, strerror(errno));
        fclose(file);
        return -1;
    }

    s->data[buffer_len] = '\0';
    s->len = buffer_len;

    fclose(file);

    return 1;
}

void string_clear(String *s)
{
    memset(s->data, 0, s->cap);
    s->len = 0;
}

void string_insert(String *s, size_t idx, char c)
{
    if (idx <= s->len)
    {
        s->len += 1;
        string_check_capacity(s);

        memmove(s->data + idx + 1, s->data + idx, s->len - idx - 1);

        s->data[idx] = c;
        s->data[s->len] = '\0';
    }
}

void string_delete(String *s, size_t idx)
{
    if (idx > 0)
    {
        memmove(s->data + idx - 1, s->data + idx, s->len - idx + 1);
        s->len -= 1;
        s->data[s->len] = '\0';
    }
}

void string_info(String *s)
{
    printf("len = %lu\n", s->len);
    printf("cap = %lu\n", s->cap);
}

typedef struct {
    const char *filepath;
    String text;
    size_t index;
    Vector2 scroll;
} Buffer;

void buffer_empty(Buffer *b)
{
    String s = {0};
    string_init(&s);
    b->filepath = "untitled";
    b->text = s;
}

void buffer_load_from_file(Buffer *b, const char *filepath)
{
    String s = {0};
    // FIXME: Check for errors
    string_from_file(&s, filepath);
    b->filepath = filepath;
    b->text = s;
}

void buffer_clear(Buffer *b)
{
    string_clear(&b->text);
    b->index = 0;
    b->scroll = (Vector2){0};
}

size_t buffer_get_col(Buffer b)
{
    size_t col = 0;

    for (size_t i = 0; i < b.index; i++)
    {
        if ((b.text.data[i] & 0xC0) != 0x80) col += 1; // Skipping utf-8 cotinuation bytes
        if (b.text.data[i] == '\n') col = 0;
    }

    return col;
}

size_t buffer_get_row(Buffer b)
{
    size_t row = 0;

    for (size_t i = 0; i < b.index; i++)
    {
        if (b.text.data[i] == '\n') row += 1;
    }

    return row;
}

void buffer_update_scroll(Buffer *b, Vector2 font_size)
{
    Vector2 cursor_pos = {
        buffer_get_col(*b) * font_size.x,
        buffer_get_row(*b) * font_size.y
    };

    if (cursor_pos.x < b->scroll.x)
        b->scroll.x = cursor_pos.x;
    else if (cursor_pos.x + font_size.x > b->scroll.x + GetScreenWidth())
        b->scroll.x = cursor_pos.x + font_size.x - GetScreenWidth();

    if (cursor_pos.y < b->scroll.y)
        b->scroll.y = cursor_pos.y;
    else if (cursor_pos.y + font_size.y > b->scroll.y + GetScreenHeight())
        b->scroll.y = cursor_pos.y + font_size.y - GetScreenHeight();
}

void buffer_insert(Buffer *b, char c)
{
    string_insert(&b->text, b->index, c);
    b->index += 1;
}

void buffer_delete(Buffer *b)
{
    if (b->index < 1) return;

    size_t char_start = b->index - 1;

    while (char_start > 0 && (b->text.data[char_start] & 0xC0) == 0x80)
        char_start -= 1;

    int byte_len = 0;
    GetCodepoint(&b->text.data[char_start], &byte_len);

    for (int i = 0; i < byte_len; i++)
        string_delete(&b->text, char_start + 1);

    b->index = char_start;
}

void buffer_move_right(Buffer *b)
{
    if (b->index >= b->text.len) return;

    b->index += 1;

    // Properly skip UTF-8 bytes
    while (b->index < b->text.len && (b->text.data[b->index] & 0xC0) == 0x80)
        b->index += 1;
}

void buffer_move_left(Buffer *b)
{
    if (b->index < 1) return;

    b->index -= 1;

    // Properly skip UTF-8 bytes
    while (b->index > 0 && (b->text.data[b->index] & 0xC0) == 0x80)
        b->index -= 1;
}

size_t buffer_get_line_len(Buffer b, size_t pos)
{
    if (pos >= b.text.len) return 0;

    size_t line_begin = pos;

    while (line_begin > 0 && b.text.data[line_begin-1] != '\n') line_begin--;

    size_t line_end = pos;

    while (line_end < b.text.len && b.text.data[line_end] != '\n') line_end++;

    return line_end - line_begin;
}

void buffer_move_down(Buffer *b)
{
    size_t line_end = b->index;
    size_t col = buffer_get_col(*b);

    while (line_end < b->text.len && b->text.data[line_end] != '\n') line_end += 1;

    if (line_end >= b->text.len) return;

    size_t new_index = line_end + 1;

    size_t current_col = 0;

    while (new_index < b->text.len && b->text.data[new_index] != '\n' && current_col < col)
    {
        if ((b->text.data[new_index] & 0xC0) != 0x80) current_col++;
        new_index++;
    }

    while (new_index < b->text.len && (b->text.data[new_index] & 0xC0) == 0x80)
    {
        new_index++;
    }

    b->index = new_index;
}

void buffer_move_up(Buffer *b)
{
    size_t line_begin = b->index;
    size_t col = buffer_get_col(*b);

    while (line_begin > 0 && b->text.data[line_begin-1] != '\n') line_begin -= 1;

    if (line_begin == 0) return;

    size_t line_above_start = line_begin - 1;
    while (
        line_above_start > 0 && b->text.data[line_above_start-1] != '\n'
    ) line_above_start -= 1;

    size_t new_index = line_above_start;
    size_t current_col = 0;

    while (new_index < line_begin - 1 && current_col < col)
    {
        if ((b->text.data[new_index] & 0xC0) != 0x80) current_col++;
        new_index++;
    }

    while (new_index < b->text.len && (b->text.data[new_index] & 0xC0) == 0x80)
    {
        new_index++;
    }

    b->index = new_index;
}

void buffer_move_line_begin(Buffer *b)
{
    if (b->index == 0) return;

    size_t diff = b->index;

    for (size_t i = b->index; i > 0; i--)
    {
        if (b->text.data[i-1] == '\n') break;

        diff -= 1;
    }

    b->index = diff;
}

void buffer_move_line_end(Buffer *b)
{
    if (b->index == b->text.len) return;

    size_t plus = b->index;

    for (size_t i = b->index; i < b->text.len; i++)
    {
        if (b->text.data[i] == '\n') break;
        plus += 1;
    }

    b->index = plus;
}

void buffer_new_line_bellow(Buffer *b)
{
    size_t line_end = b->index;

    for (size_t i = b->index; i < b->text.len; i++)
    {
        line_end = i;
        if (b->text.data[i] == '\n') break;
    }

    string_insert(&b->text, line_end, '\n');
    b->index = line_end+1;
}

void buffer_new_line_above(Buffer *b)
{
    size_t line_start = b->index;

    while (line_start > 0 && b->text.data[line_start-1] != '\n')
    {
        line_start -= 1;
    }

    string_insert(&b->text, line_start, '\n');
    b->index = line_start;
}

void buffer_move_next_word(Buffer *b)
{
    if (b->index >= b->text.len) return;

    size_t next_word = b->index;

    // Cursor is on top of characters
    while (next_word < b->text.len && !isspace(b->text.data[next_word]))
    {
        next_word += 1;
    }

    // Cursor reached empty line
    if (
        next_word < b->text.len &&
        b->text.data[next_word] == '\n' && b->text.data[next_word+1] == '\n'
    ) {
        next_word += 1;
        b->index = next_word;
        return;
    }

    // Cursor is on top of whitespace
    while (next_word < b->text.len && isspace(b->text.data[next_word]))
    {
        next_word += 1;
    }

    b->index = next_word;
}

void buffer_move_prev_word(Buffer *b)
{
    if (b->index < 1) return;

    size_t prev_word = b->index;

    // Cursor is on top of whitespace
    while (prev_word > 0 && isspace(b->text.data[prev_word-1]))
    {
        if (
            prev_word >= 2 &&
            b->text.data[prev_word-1] == '\n' && isspace(b->text.data[prev_word-2])
        ) {
            b->index = prev_word-1;
            return;
        }
        prev_word -= 1;
    }

    // Cursor is on top of characters
    while (prev_word > 0 && !isspace(b->text.data[prev_word-1]))
    {
        prev_word -= 1;
    }

    b->index = prev_word;
}

void draw_characters(Font font, const char *text, Vector2 origin, Vector2 font_size, Vector2 scroll)
{
    Vector2 cell_pos = {-scroll.x, -scroll.y};
    cell_pos = Vector2Add(cell_pos, origin);

    size_t i = 0;

    while (text[i] != '\0')
    {
        int byte_len = 0;
        int codepoint = GetCodepoint(&text[i], &byte_len);

        if (codepoint == '\n')
        {
            cell_pos.x = -scroll.x + origin.x;
            cell_pos.y += font_size.y;
            i += byte_len;
            continue;
        }

        DrawTextCodepoint(font, codepoint, cell_pos, FONT_SIZE, GetColor(COLOR_FG));

        cell_pos.x += font_size.x;
        i += byte_len;
    }
}

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT,
    MODE_COMMAND,
} Mode;

#define BUFFER_LIST_CAP 16

typedef struct {
    Buffer buffers[BUFFER_LIST_CAP];
    size_t buffers_len;
    size_t active_buffer;

    Font font;
    Vector2 font_size;
    Rectangle command_bounds;
    Buffer command_buffer;
    int command_padding;
    Mode mode;
    Rectangle cursor;
} Editor;

void editor_init(Editor *edt)
{
    const char *font_path = "resources/DepartureMono/DepartureMono-Regular.otf";
    edt->font = LoadFontEx(font_path, FONT_SIZE, 0, CODEPOINT_LEN);
    edt->font_size = MeasureTextEx(edt->font, "X", FONT_SIZE, 0);

    Buffer cmd = {0};
    buffer_empty(&cmd);
    edt->command_buffer = cmd;
    edt->command_padding = (int)edt->font_size.x / 1.5;

    edt->mode = MODE_NORMAL;

    edt->cursor.width = edt->font_size.x;
    edt->cursor.height = edt->font_size.y;
}

void editor_new_buffer(Editor *edt)
{
    Buffer buf = {0};
    buffer_empty(&buf);

    edt->buffers[edt->buffers_len++] = buf;
}

void editor_load_file(Editor *edt, const char *file_path)
{
    Buffer buf = {0};
    buffer_load_from_file(&buf, file_path);

    edt->buffers[edt->buffers_len++] = buf;
}

void editor_update_cursor(Editor *edt)
{
    Buffer *buf = &edt->buffers[edt->active_buffer];

    if (edt->mode == MODE_COMMAND)
    {
        edt->cursor.x = (edt->command_bounds.x + edt->command_padding) + edt->font_size.x + buffer_get_col(edt->command_buffer) * edt->font_size.x - buf->scroll.x;
        edt->cursor.y = (edt->command_bounds.y + edt->command_padding) + buffer_get_row(edt->command_buffer) * edt->font_size.y - buf->scroll.y;
    } else {
        edt->cursor.x = buffer_get_col(*buf) * edt->font_size.x - buf->scroll.x;
        edt->cursor.y = buffer_get_row(*buf) * edt->font_size.y - buf->scroll.y;
    }

    edt->cursor.width = edt->mode == MODE_NORMAL ? edt->font_size.x : edt->font_size.x / 6;
}

float key_down_timer = 0.0;
float key_down_repeat_time = 0.4;

void handle_normal_mode(Editor *edt)
{
    Buffer *buf = &edt->buffers[edt->active_buffer];

    if (IsKeyPressed(KEY_RIGHT))
    {
        edt->active_buffer =
            edt->active_buffer >= edt->buffers_len - 1
            ? 0
            : edt->active_buffer+1;
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        edt->active_buffer =
            edt->active_buffer < 1
            ? edt->buffers_len - 1
            : edt->active_buffer - 1;
    }

    if (IsKeyPressed(KEY_I))
    {
        edt->mode = MODE_INSERT;
    }

    if (IsKeyDown(KEY_L))
    {
        key_down_timer += GetFrameTime();
        if (IsKeyPressed(KEY_L)) buffer_move_right(buf);
        if (key_down_timer >= key_down_repeat_time) buffer_move_right(buf);
    }

    if (IsKeyDown(KEY_H))
    {
        key_down_timer += GetFrameTime();
        if (IsKeyPressed(KEY_H)) buffer_move_left(buf);
        if (key_down_timer >= key_down_repeat_time) buffer_move_left(buf);
    }

    if (IsKeyDown(KEY_J))
    {
        key_down_timer += GetFrameTime();
        if (IsKeyPressed(KEY_J)) buffer_move_down(buf);
        if (key_down_timer >= key_down_repeat_time) buffer_move_down(buf);
    }

    if (IsKeyDown(KEY_K))
    {
        key_down_timer += GetFrameTime();
        if (IsKeyPressed(KEY_K)) buffer_move_up(buf);
        if (key_down_timer >= key_down_repeat_time) buffer_move_up(buf);
    }

    if (IsKeyReleased(KEY_L)) key_down_timer = 0;
    if (IsKeyReleased(KEY_H)) key_down_timer = 0;
    if (IsKeyReleased(KEY_J)) key_down_timer = 0;
    if (IsKeyReleased(KEY_K)) key_down_timer = 0;

    if (IsKeyPressed(KEY_ZERO)) buffer_move_line_begin(buf);

    if (IsKeyPressed(KEY_O) && !IsKeyDown(KEY_LEFT_SHIFT))
    {
        buffer_new_line_bellow(buf);
        edt->mode = MODE_INSERT;
    }

    if (IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_LEFT_SHIFT))
    {
        if (IsKeyPressed(KEY_SEMICOLON))
        {
            edt->mode = MODE_COMMAND;
        }

        if (IsKeyPressed(KEY_W)) buffer_move_next_word(buf);
        if (IsKeyPressed(KEY_B)) buffer_move_prev_word(buf);

        if (IsKeyPressed(KEY_A))
        {
            buffer_move_line_end(buf);
            edt->mode = MODE_INSERT;
        }
        if (IsKeyPressed(KEY_I))
        {
            buffer_move_line_begin(buf);
            edt->mode = MODE_INSERT;
        }
        if (IsKeyPressed(KEY_FOUR)) buffer_move_line_end(buf);
        if (IsKeyPressed(KEY_O))
        {
            buffer_new_line_above(buf);
            edt->mode = MODE_INSERT;
        }
    }
}

void handle_insert_mode(Editor *edt)
{
    int codepoint = GetCharPressed();

    Buffer *buf = &edt->buffers[edt->active_buffer];

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK))
    {
        edt->mode = MODE_NORMAL;
        if (buf->text.data[buf->index-1] != '\n') buffer_move_left(buf);
    }

    if (IsKeyDown(KEY_RIGHT_CONTROL) || IsKeyDown(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_C))
        {
            edt->mode = MODE_NORMAL;
            if (buf->text.data[buf->index-1] != '\n') buffer_move_left(buf);
        }
    }

    if (IsKeyPressed(KEY_ENTER)) buffer_insert(buf, '\n');

    if (IsKeyPressed(KEY_TAB))
    {
        for (int i = 0; i < TAB_SIZE; i++) buffer_insert(buf, ' ');
    }

    if (IsKeyPressed(KEY_BACKSPACE)) buffer_delete(buf);

    while (codepoint > 0)
    {
        int len = 0;
        const char *char_encoded = CodepointToUTF8(codepoint, &len);
        if (codepoint >= 32 && codepoint <= CODEPOINT_LEN)
        {
            for (int i = 0; i < len; i++) buffer_insert(buf, char_encoded[i]);
        }
        codepoint = GetCharPressed();
    }
}

void handle_command_mode(Editor *edt)
{
    int codepoint = GetCharPressed();

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK))
    {
        edt->mode = MODE_NORMAL;
        if (edt->command_buffer.text.len > 0) buffer_clear(&edt->command_buffer);
    }

    if (IsKeyPressed(KEY_BACKSPACE)) buffer_delete(&edt->command_buffer);

    while (codepoint > 0)
    {
        int len = 0;
        const char *char_encoded = CodepointToUTF8(codepoint, &len);
        if (codepoint >= 32 && codepoint <= CODEPOINT_LEN)
        {
            for (int i = 0; i < len; i++) buffer_insert(&edt->command_buffer, char_encoded[i]);
        }
        codepoint = GetCharPressed();
    }
}

int main(int argc, char **argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Codigo");
    SetTargetFPS(75);

    EnableEventWaiting();
    SetExitKey(0);

    Editor editor = {0};
    editor_init(&editor);

    // TODO: Do some arguments parsing
    if (argc > 1) editor_load_file(&editor, argv[1]);
    else editor_new_buffer(&editor);

    editor_load_file(&editor, "Makefile");
    editor_load_file(&editor, "resources/UTF-8-demo.txt");

    while (!WindowShouldClose())
    {
        if (editor.mode == MODE_NORMAL)
        {
            handle_normal_mode(&editor);
        }
        else if (editor.mode == MODE_INSERT)
        {
            handle_insert_mode(&editor);
        }
        else if (editor.mode == MODE_COMMAND)
        {
            handle_command_mode(&editor);
        }

        if (editor.mode == MODE_COMMAND)
        {
            float width_factor = 1.5;
            int padding = editor.command_padding;
            editor.command_bounds.width  = GetScreenWidth() / width_factor + (padding*2);
            editor.command_bounds.height = editor.font_size.y + (padding*2);
            editor.command_bounds.x      = editor.command_bounds.width / 4 - padding;
            editor.command_bounds.y      = GetScreenHeight() / 5 - padding;
        }

        Buffer *buf = &editor.buffers[editor.active_buffer];

        buffer_update_scroll(buf, editor.font_size);
        editor_update_cursor(&editor);

        BeginDrawing();

        ClearBackground(GetColor(COLOR_BG));

        // Cursor
        if (editor.mode != MODE_COMMAND) DrawRectangleRec(editor.cursor, GetColor(COLOR_BLUE));

        // Text
        draw_characters(editor.font, buf->text.data, (Vector2){0}, editor.font_size, buf->scroll);

        if (editor.mode == MODE_COMMAND)
        {
            float thicc = 2.0;
            Rectangle border_rect = {0};
            border_rect.x      = editor.command_bounds.x - thicc;
            border_rect.y      = editor.command_bounds.y - thicc;
            border_rect.width  = editor.command_bounds.width + thicc * 2;
            border_rect.height = editor.command_bounds.height + thicc * 2;

            // Border
            DrawRectangleLinesEx(border_rect, 2.0, GetColor(COLOR_FG));

            // Command background
            DrawRectangleRec(editor.command_bounds, GetColor(COLOR_CMD));

            // Cursor
            DrawRectangleRec(editor.cursor, GetColor(COLOR_BLUE));

            // Prompt
            Vector2 prompt_pos = {
                editor.command_bounds.x + editor.command_padding,
                editor.command_bounds.y + editor.command_padding
            };
            DrawTextCodepoint(editor.font, ':', prompt_pos, FONT_SIZE, GetColor(COLOR_FG));

            // Text
            Vector2 text_origin = {
                editor.command_bounds.x + editor.font_size.x + editor.command_padding,
                editor.command_bounds.y + editor.command_padding
            };
            draw_characters(
                editor.font,
                editor.command_buffer.text.data,
                text_origin,
                editor.font_size,
                (Vector2){-0,-0}
            );
        }

        EndDrawing();
    }

    return 0;
}

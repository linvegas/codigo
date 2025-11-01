#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define FONT_SIZE 33
#define TAB_SIZE 4
#define CODEPOINT_LEN 250
#define STRING_CAP 1024*16

// Inspired by alabaster.nvim colorscheme
// https://sr.ht/~p00f/alabaster.nvim/
#define COLOR_BG   0x0e1415ff
#define COLOR_FG   0xcececeff
#define COLOR_BLUE 0x007accff

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT,
} Mode;

typedef struct {
    char   data[STRING_CAP];
    size_t len;
} String;

void string_init(String *s)
{
    memset(s, 0, sizeof(char)*STRING_CAP);
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

    size_t buffer_len = fread(s->data, 1, (size_t)file_len ,file);

    if (ferror(file) && !feof(file)) {
        fprintf(stderr, "[ERROR] Tried to read '%s': %s\n", file_path, strerror(errno));
        fclose(file);
        return -1;
    }

    s->data[buffer_len+1] = '\0';
    s->len = buffer_len;

    fclose(file);

    return 1;
}

void string_insert(String *s, size_t idx, char c)
{
    if (idx <= s->len && s->len < STRING_CAP)
    {
        memmove(s->data + idx + 1, s->data + idx, s->len - idx);
        s->data[idx] = c;
        s->len += 1;
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

typedef struct {
    String  text;
    size_t  index;
    Vector2 scroll;
    const char *name;
} Document;

void doc_empty(Document *d)
{
    String s = {0};
    string_init(&s);
    d->name = "Untitled";
    d->text = s;
}

void doc_load_file(Document *d, const char *file_path)
{
    String s = {0};
    string_from_file(&s, file_path);
    d->name = file_path;
    d->text = s;
}

size_t doc_get_col(Document d)
{
    size_t col = 0;

    for (size_t i = 0; i < d.index; i++)
    {
        if ((d.text.data[i] & 0xC0) != 0x80) col += 1; // Skipping utf-8 cotinuation bytes
        if (d.text.data[i] == '\n') col = 0;
    }

    return col;
}

size_t doc_get_row(Document d)
{
    size_t row = 0;

    for (size_t i = 0; i < d.index; i++)
    {
        if (d.text.data[i] == '\n') row += 1;
    }

    return row;
}

void doc_update_scroll(Document *d, Vector2 font_size)
{
    Vector2 cursor_pos = {
        doc_get_col(*d) * font_size.x,
        doc_get_row(*d) * font_size.y
    };

    if (cursor_pos.x < d->scroll.x)
        d->scroll.x = cursor_pos.x;
    else if (cursor_pos.x + font_size.x > d->scroll.x + GetScreenHeight())
        d->scroll.x = cursor_pos.x + font_size.x - GetScreenHeight();

    if (cursor_pos.y < d->scroll.y)
        d->scroll.y = cursor_pos.y;
    else if (cursor_pos.y + font_size.y > d->scroll.y + GetScreenHeight())
        d->scroll.y = cursor_pos.y + font_size.y - GetScreenHeight();
}

void doc_insert(Document *d, char c)
{
    string_insert(&d->text, d->index, c);
    d->index += 1;
}

void doc_delete(Document *d)
{
    if (d->index < 1) return;

    size_t char_start = d->index - 1;

    while (char_start > 0 && (d->text.data[char_start] & 0xC0) == 0x80)
        char_start -= 1;

    int byte_len = 0;
    GetCodepoint(&d->text.data[char_start], &byte_len);

    for (int i = 0; i < byte_len; i++)
        string_delete(&d->text, char_start + 1);

    d->index = char_start;
}

void doc_move_right(Document *d)
{
    if (d->index >= d->text.len) return;

    d->index += 1;

    // Properly skip UTF-8 bytes
    while (d->index < d->text.len && (d->text.data[d->index] & 0xC0) == 0x80)
        d->index += 1;
}

void doc_move_left(Document *d)
{
    if (d->index < 1) return;

    d->index -= 1;

    // Properly skip UTF-8 bytes
    while (d->index > 0 && (d->text.data[d->index] & 0xC0) == 0x80)
        d->index -= 1;
}

size_t doc_get_line_len(Document d, size_t pos)
{
    if (pos >= d.text.len) return 0;

    size_t line_begin = pos;

    while (line_begin > 0 && d.text.data[line_begin-1] != '\n') line_begin--;

    size_t line_end = pos;

    while (line_end < d.text.len && d.text.data[line_end] != '\n') line_end++;

    return line_end - line_begin;
}

void doc_move_down(Document *d)
{
    size_t line_end = d->index;
    size_t col = doc_get_col(*d);

    while (line_end < d->text.len && d->text.data[line_end] != '\n') line_end += 1;

    if (line_end >= d->text.len) return;

    size_t new_index = line_end + 1;

    size_t current_col = 0;

    while (new_index < d->text.len && d->text.data[new_index] != '\n' && current_col < col)
    {
        if ((d->text.data[new_index] & 0xC0) != 0x80) current_col++;
        new_index++;
    }

    while (new_index < d->text.len && (d->text.data[new_index] & 0xC0) == 0x80)
    {
        new_index++;
    }

    d->index = new_index;

    // size_t line_bellow_len = doc_get_line_len(*d, line_end+1);

    // if (col < line_bellow_len)
    //     d->index = (line_end+1) + col;
    // else
    //     d->index = (line_end+1) + line_bellow_len;
}

void doc_move_up(Document *d)
{
    size_t line_begin = d->index;
    size_t col = doc_get_col(*d);

    while (line_begin > 0 && d->text.data[line_begin-1] != '\n') line_begin -= 1;

    if (line_begin == 0) return;

    size_t line_above_start = line_begin - 1;
    while (line_above_start > 0 && d->text.data[line_above_start-1] != '\n') line_above_start -= 1;

    size_t new_index = line_above_start;
    size_t current_col = 0;

    while (new_index < line_begin - 1 && current_col < col)
    {
        if ((d->text.data[new_index] & 0xC0) != 0x80) current_col++;
        new_index++;
    }

    while (new_index < d->text.len && (d->text.data[new_index] & 0xC0) == 0x80)
    {
        new_index++;
    }

    d->index = new_index;

    // size_t line_above_len = doc_get_line_len(*d, line_begin-1);

    // if (col < line_above_len)
    //     d->index = (line_begin-1) - (line_above_len - col);
    // else
    //     d->index = line_begin - 1;
}

void doc_move_line_begin(Document *d)
{
    if (d->index == 0) return;

    size_t diff = d->index;

    for (size_t i = d->index; i > 0; i--)
    {
        if (d->text.data[i-1] == '\n') break;

        diff -= 1;
    }

    d->index = diff;
}

void doc_move_line_end(Document *d)
{
    if (d->index == d->text.len) return;

    size_t plus = d->index;

    for (size_t i = d->index; i < d->text.len; i++)
    {
        if (d->text.data[i] == '\n') break;
        plus += 1;
    }

    d->index = plus;
}

void doc_new_line_bellow(Document *d)
{
    size_t line_end = d->index;

    for (size_t i = d->index; i < d->text.len; i++)
    {
        line_end = i;
        if (d->text.data[i] == '\n') break;
    }

    string_insert(&d->text, line_end, '\n');
    d->index = line_end+1;
}

void doc_new_line_above(Document *d)
{
    size_t line_start = d->index;

    while (line_start > 0 && d->text.data[line_start-1] != '\n')
    {
        line_start -= 1;
    }

    string_insert(&d->text, line_start, '\n');
    d->index = line_start;
}

void draw_cells(Font font, const char *text, Vector2 font_size, Vector2 scroll)
{
    Vector2 cell_pos = {-scroll.x, -scroll.y};

    // int first_visible_line = (int)(scroll.y / font_size.y);
    // int last_visible_line = (int)((scroll.y + GetScreenHeight()) / font_size.y) + 1;

    size_t i = 0;
    // int current_line = 0;

    // while (text[i] != '\0' && current_line < first_visible_line)
    // {
    //     if (text[i] == '\n') current_line += 1;
    //     i += 1;
    // }

    // cell_pos.y = current_line * font_size.y - scroll.y;

    while (text[i] != '\0'/* && current_line <= last_visible_line*/)
    {
        int byte_len = 0;
        int codepoint = GetCodepoint(&text[i], &byte_len);

        if (codepoint == '\n')
        {
            // cell_pos.x = 0;
            // cell_pos.y += font_size.y;
            // current_line += 1;
            cell_pos.x = -scroll.x;
            cell_pos.y += font_size.y;
            i += byte_len;
            continue;
        }

        DrawTextCodepoint(font, codepoint, cell_pos, FONT_SIZE, GetColor(COLOR_FG));

        cell_pos.x += font_size.x;
        i += byte_len;
    }
}

int main(int argc, char **argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    // SetTraceLogLevel(LOG_NONE);
    InitWindow(800, 600, "Codigo");
    SetExitKey(0);

    const char *font_path = "resources/DepartureMono/DepartureMono-Regular.otf";
    Font font = LoadFontEx(font_path, FONT_SIZE, 0, CODEPOINT_LEN);

    Vector2 font_size = MeasureTextEx(font, "X", FONT_SIZE, 0);

    Document doc = {0};

    if (argc > 1) doc_load_file(&doc, argv[1]);
    else doc_empty(&doc);

    Vector2 cursor_pos = {0};
    Vector2 cursor_size = font_size;

    Mode mode = MODE_NORMAL;

    SetTargetFPS(75);

    EnableEventWaiting();

    float key_down_timer = 0.0;
    float key_down_repeat_time = 0.2;

    while (!WindowShouldClose())
    {
        int codepoint = GetCharPressed();

        if (mode == MODE_NORMAL)
        {
            if (IsKeyPressed(KEY_I))
            {
                mode = MODE_INSERT;
                codepoint = 0;
            }

            if (IsKeyDown(KEY_L))
            {
                key_down_timer += GetFrameTime();
                if (IsKeyPressed(KEY_L)) doc_move_right(&doc);
                if (key_down_timer >= key_down_repeat_time) doc_move_right(&doc);
            }

            if (IsKeyDown(KEY_H))
            {
                key_down_timer += GetFrameTime();
                if (IsKeyPressed(KEY_H)) doc_move_left(&doc);
                if (key_down_timer >= key_down_repeat_time) doc_move_left(&doc);
            }

            if (IsKeyDown(KEY_J))
            {
                key_down_timer += GetFrameTime();
                if (IsKeyPressed(KEY_J)) doc_move_down(&doc);
                if (key_down_timer >= key_down_repeat_time) doc_move_down(&doc);
            }

            if (IsKeyDown(KEY_K))
            {
                key_down_timer += GetFrameTime();
                if (IsKeyPressed(KEY_K)) doc_move_up(&doc);
                if (key_down_timer >= key_down_repeat_time) doc_move_up(&doc);
            }

            if (IsKeyReleased(KEY_L)) key_down_timer = 0;
            if (IsKeyReleased(KEY_H)) key_down_timer = 0;
            if (IsKeyReleased(KEY_J)) key_down_timer = 0;
            if (IsKeyReleased(KEY_K)) key_down_timer = 0;

            if (IsKeyPressed(KEY_ZERO)) doc_move_line_begin(&doc);

            if (IsKeyPressed(KEY_O) && !IsKeyDown(KEY_LEFT_SHIFT))
            {
                doc_new_line_bellow(&doc);
                mode = MODE_INSERT;
                codepoint = 0;
            }

            if (IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_LEFT_SHIFT))
            {
                if (IsKeyPressed(KEY_A))
                {
                    doc_move_line_end(&doc);
                    mode = MODE_INSERT;
                    codepoint = 0;
                }
                if (IsKeyPressed(KEY_I))
                {
                    doc_move_line_begin(&doc);
                    mode = MODE_INSERT;
                    codepoint = 0;
                }
                if (IsKeyPressed(KEY_FOUR)) doc_move_line_end(&doc);
                if (IsKeyPressed(KEY_O))
                {
                    doc_new_line_above(&doc);
                    mode = MODE_INSERT;
                    codepoint = 0;
                }
            }
        }

        if (mode == MODE_INSERT)
        {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_CAPS_LOCK))
            {
                mode = MODE_NORMAL;
                codepoint = 0;
                if (doc.text.data[doc.index-1] != '\n') doc_move_left(&doc);
            }

            if (IsKeyDown(KEY_RIGHT_CONTROL) || IsKeyDown(KEY_LEFT_CONTROL))
            {
                if (IsKeyPressed(KEY_C))
                {
                    mode = MODE_NORMAL;
                    codepoint = 0;
                    if (doc.text.data[doc.index-1] != '\n') doc_move_left(&doc);
                }
            }

            if (IsKeyPressed(KEY_ENTER)) doc_insert(&doc, '\n');

            if (IsKeyPressed(KEY_TAB))
            {
                for (int i = 0; i < TAB_SIZE; i++) doc_insert(&doc, ' ');
            }

            if (IsKeyPressed(KEY_BACKSPACE)) doc_delete(&doc);

            while (codepoint > 0)
            {
                int len = 0;
                const char *char_encoded = CodepointToUTF8(codepoint, &len);
                if (codepoint >= 32 && codepoint <= CODEPOINT_LEN)
                {
                    for (int i = 0; i < len; i++) doc_insert(&doc, char_encoded[i]);
                }
                codepoint = GetCharPressed();
            }
        }

        doc_update_scroll(&doc, font_size);

        cursor_pos.x = doc_get_col(doc) * font_size.x - doc.scroll.x;
        cursor_pos.y = doc_get_row(doc) * font_size.y - doc.scroll.y;
        cursor_size.x = mode == MODE_NORMAL ? font_size.x : font_size.x / 6;

        BeginDrawing();

        ClearBackground(GetColor(COLOR_BG));

        DrawRectangleV(cursor_pos, cursor_size, GetColor(COLOR_BLUE));

        draw_cells(font, doc.text.data, font_size, doc.scroll);

        EndDrawing();
    }
}

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "buffer.h"

Line *line_new() {
    Line *line = (Line *)malloc(sizeof(Line));

    assert(line != NULL);

    line->text = (char *)calloc(LINE_INIT_CAP, sizeof(char));

    assert(line->text != NULL);

    line->cap = LINE_INIT_CAP;
    line->len = 0;

    return line;
}

void line_grow(Line *line)
{
    while (line->len >= line->cap) {
        size_t new_capacity = line->cap * 2;

        line->text = realloc(line->text, new_capacity * sizeof(char));

        assert(line->text != NULL && "line->text realloc failed");

        line->cap = new_capacity;
    }
}

Line *line_from(const char *text)
{
    Line *line = line_new();

    size_t text_len = strlen(text);

    line->len = text_len;

    line_grow(line);

    memcpy(line->text, text, text_len);

    return line;
}

void line_insert_at(Line *line, size_t col, const char *text)
{
    const size_t text_len = strlen(text);

    if (col > line->len || text_len == 0) return;

    line_grow(line);

    memmove(
        line->text + col + text_len,
        line->text + col,
        (line->len - col) * sizeof(char)
    );

    memcpy(line->text + col, text, text_len);
    // line->text[col] = text;
    line->len += text_len;
}

void line_delete_at(Line *line, size_t col)
{
    if (col > line->len) col = line->len;

    memmove(
        line->text + col,
        line->text + col + 1,
        (line->len - col) * sizeof(char)
    );

    line->len -= 1;
}

void line_free(Line *line)
{
    free(line);
}

Buffer *buffer_new(const char *name)
{
    Buffer *buf = (Buffer *)malloc(sizeof(Buffer));

    assert(buf != NULL);

    buf->lines = (Line **)calloc(BUF_INIT_CAP, sizeof(Line));

    assert(buf->lines != NULL);

    buf->lines[0] = line_new();
    buf->len = 1;

    buf->cap = BUF_INIT_CAP;
    buf->cursor_row = 0;
    buf->cursor_col = 0;
    buf->cursor_prev_col = 0;
    buf->scroll_row = 0;
    buf->scroll_col = 0;

    buf->name = (name == NULL || *name == '\0') ? "[NO NAME]" : name;

    buf->view = (Rectangle){0};

    return buf;
}

void buffer_grow(Buffer *buf)
{
    while (buf->len >= buf->cap) {
        size_t new_capacity = buf->cap * 2;

        buf->lines = (Line **)realloc(buf->lines, new_capacity * sizeof(Line));

        assert(buf->lines != NULL && "buf->lines realloc failed");

        buf->cap = new_capacity;
    }
}

Buffer *buffer_from_file(const char *filename)
{
    Buffer *buf = buffer_new("main.c");
    buf->len = 0;
    buf->lines[0] = NULL;

    FILE *file =  fopen(filename, "rb");

    char line[1024];

    size_t i = 0;

    while (fgets(line, sizeof(line), file)) {
        buffer_grow(buf);

        size_t line_len = strlen(line);

        if (line[line_len - 1] == '\n') line[line_len - 1] = '\0';

        Line *new_line = line_from(line);

        buf->lines[i] = new_line;
        buf->len += 1;

        i++;
    }

    fclose(file);

    return buf;
}

// TODO: Reimplement this taking in count the row position
// and doing the proper memory allocation
void buffer_new_line(Buffer *buf)
{
    if (buf->cursor_row > buf->len) return;

    buffer_grow(buf);

    buf->cursor_row += 1;
    buf->cursor_col = 0;
    buf->cursor_prev_col = buf->cursor_col;

    buf->lines[buf->cursor_row] = line_new();
    buf->len += 1;
}

void buffer_insert_text(Buffer *buf, const char *text)
{
    assert(buf->cursor_row < buf->len);

    line_insert_at(buf->lines[buf->cursor_row], buf->cursor_col, text);

    buf->cursor_col += 1;
    buf->cursor_prev_col = buf->cursor_col;
}

void buffer_delete_line(Buffer *buf, Line *line)
{
    if (buf->len < 2) return;

    memmove(
        buf->lines + buf->cursor_row,
        buf->lines + buf->cursor_row + 1,
        (buf->len - buf->cursor_row) * sizeof(line)
    );

    buf->len -= 1;
    line_free(line);
}

void buffer_delete_text(Buffer *buf)
{
    if (buf->cursor_col > 0 && buf->lines[buf->cursor_row]->len > 0) {
        line_delete_at(buf->lines[buf->cursor_row], buf->cursor_col - 1);

        buf->cursor_col -= 1;
        buf->cursor_prev_col = buf->cursor_col;

    } else if (buf->lines[buf->cursor_row]->len < 1 && buf->len > 1) {
        buffer_delete_line(buf, buf->lines[buf->cursor_row]);

        if (buf->cursor_row > 0) buf->cursor_row -= 1;

        buf->cursor_col = buf->lines[buf->cursor_row]->len;
        buf->cursor_prev_col = buf->cursor_col;
    }
}

void buffer_delete_text_under_cursor(Buffer *buf)
{
    if (buf->lines[buf->cursor_row]->len > 0)
        line_delete_at(buf->lines[buf->cursor_row], buf->cursor_col);
}

void buffer_move_cursor_left(Buffer *buf)
{
    if (buf->cursor_col == 0) {
        buf->cursor_col = 0;
    } else {
        buf->cursor_col -= 1;
        buf->cursor_prev_col = buf->cursor_col;
    }
}

void buffer_move_cursor_right(Buffer *buf)
{
    size_t cur_line_len = buf->lines[buf->cursor_row]->len;

    if (buf->cursor_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col += 1;
        buf->cursor_prev_col = buf->cursor_col;
    }
}

void buffer_move_cursor_up(Buffer *buf, Vector2 font_size)
{
    if (buf->cursor_row == 0) {
        buf->cursor_row = 0;
    } else {
        buf->cursor_row -= 1;
    }

    size_t cur_line_len = buf->lines[buf->cursor_row]->len;

    if (cur_line_len == 0) {
        buf->cursor_col = 0;
    }

    if (buf->cursor_prev_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col = buf->cursor_prev_col;
    }

    if (buf->cursor_row * font_size.y <= buf->view.y + 100 && buf->view.y > 0) {
        buf->scroll_row -= 1;
        buf->view.y = buf->scroll_row * font_size.y;
    }
}

void buffer_move_cursor_down(Buffer *buf, Vector2 font_size)
{
    if (buf->cursor_row >= buf->len - 1) {
        buf->cursor_row = buf->len - 1;
    } else {
        buf->cursor_row += 1;
    }

    size_t cur_line_len = buf->lines[buf->cursor_row]->len;

    if (cur_line_len == 0) {
        buf->cursor_col = 0;
    }

    if (buf->cursor_prev_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col = buf->cursor_prev_col;
    }

    if (buf->cursor_row * font_size.y >= (buf->view.height + buf->view.y) - 100) {
        buf->scroll_row += 1;
        buf->view.y = buf->scroll_row * font_size.y;
    }
}

void buffer_move_cursor_line_begin(Buffer *buf)
{
    buf->cursor_col = 0;
    buf->cursor_prev_col = buf->cursor_col;
}

void buffer_move_cursor_line_end(Buffer *buf)
{
    buf->cursor_col = buf->lines[buf->cursor_row]->len;
    buf->cursor_prev_col = buf->cursor_col;
}

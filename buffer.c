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

Line *line_from(const char *text)
{
    Line *line = line_new();

    size_t text_len = strlen(text);

    line->len = text_len;

    memcpy(line->text, text, text_len);

    return line;
}

void line_insert(Line *line, const char *text, size_t col)
{
    const size_t text_len = strlen(text);

    if (col > line->len || text_len == 0) return;

    if (line->len >= line->cap) {
        size_t new_capacity = line->cap * 2;

        line->text = realloc(line->text, new_capacity * sizeof(char));

        assert(line->text != NULL && "line->text realloc failed");

        line->cap = new_capacity;
    }

    memmove(
        line->text + col + text_len,
        line->text + col,
        (line->len - col) * sizeof(char)
    );

    memcpy(line->text + col, text, text_len);
    // line->text[col] = text;
    line->len += text_len;
}

void line_delete(Line *line, size_t col)
{
    if (col > line->len) col = line->len;

    memmove(
        line->text + col,
        line->text + col + 1,
        (line->len - col) * sizeof(char)
    );

    line->len -= 1;
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

    buf->name = (name == NULL || *name == '\0') ? "[NO NAME]" : name;

    return buf;
}

void buffer_grow(Buffer *buf)
{
    if (buf->len >= buf->cap) {
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
    buf->cursor_row = 0;
    buf->cursor_col = 0;
    buf->cursor_prev_col = 0;
    buf->scroll_row = 0;
    buf->scroll_col = 0;
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

void buffer_new_line(Buffer *buf)
{
    if (buf->cursor_row > buf->len) return;

    buffer_grow(buf);
    // if (buf->len >= buf->cap) {
    //     size_t new_capacity = buf->cap * 2;

    //     buf->lines = (Line **)realloc(buf->lines, new_capacity * sizeof(Line));

    //     assert(buf->lines != NULL && "buf->lines realloc failed");

    //     buf->cap = new_capacity;
    // }

    buf->cursor_row += 1;
    buf->cursor_col = 0;
    buf->cursor_prev_col = buf->cursor_col;

    buf->lines[buf->cursor_row] = line_new();
    buf->len += 1;
}

void buffer_insert_text(Buffer *buf, const char *text)
{
    assert(buf->cursor_row < buf->len);

    line_insert(buf->lines[buf->cursor_row], text, buf->cursor_col);

    buf->cursor_col += 1;
    buf->cursor_prev_col = buf->cursor_col;
}

void buffer_delete_text(Buffer *buf)
{
    if (buf->cursor_col > 0 && buf->lines[buf->cursor_row]->len > 0) {
        line_delete(buf->lines[buf->cursor_row], buf->cursor_col - 1);
        buf->cursor_col -= 1;
        buf->cursor_prev_col = buf->cursor_col;
    }
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

void buffer_move_cursor_up(Buffer *buf)
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
}

void buffer_move_cursor_down(Buffer *buf)
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
}

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

void buffer_grow(Buffer *buf)
{
    while (buf->len >= buf->cap) {
        size_t new_capacity = buf->cap * 2;

        buf->lines = (Line **)realloc(buf->lines, new_capacity * sizeof(Line));

        assert(buf->lines != NULL && "buf->lines realloc failed");

        buf->cap = new_capacity;
    }
}

Buffer *buffer_new(const char *name, Vector2 font_size)
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
    buf->font_size = font_size;

    return buf;
}

Buffer *buffer_from_file(const char *filename, Vector2 font_size)
{
    Buffer *buf = buffer_new("main.c", font_size);
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

void buffer_new_line(Buffer *buf)
{
    if (buf->cursor_row > buf->len) return;

    buffer_grow(buf);

    Line *cur_line = buf->lines[buf->cursor_row];

    Line *new_line = NULL;

    if (cur_line->len > 0 && buf->cursor_col < cur_line->len) {
        size_t text_len = strlen(cur_line->text);

        size_t text_after_cursor_len = text_len - buf->cursor_col;

        char *text_after_cursor = calloc(text_after_cursor_len+1, sizeof(char));

        strncpy(text_after_cursor, cur_line->text + buf->cursor_col, text_after_cursor_len);

        text_after_cursor[text_after_cursor_len+1] = '\0';

        memmove(
            cur_line->text + buf->cursor_col,
            cur_line->text + buf->cursor_col + text_after_cursor_len,
            (text_len - buf->cursor_col - text_after_cursor_len) * sizeof(char)
        );

        cur_line->len -= text_after_cursor_len;
        cur_line->text[cur_line->len] = '\0';

        new_line = line_from(text_after_cursor);

        free(text_after_cursor);
    } else {
        new_line = line_new();
    }

    memmove(
        buf->lines + buf->cursor_row + 1,
        buf->lines + buf->cursor_row,
        (buf->len - buf->cursor_row) * sizeof(Line *)
    );

    buf->lines[buf->cursor_row+1] = new_line;
    buf->cursor_row += 1;
    buf->cursor_col = 0;
    buf->cursor_prev_col = buf->cursor_col;

    buf->len += 1;
}

void buffer_insert_text(Buffer *buf, const char *text)
{
    assert(buf->cursor_row < buf->len);

    line_insert_at(cur_line(buf), buf->cursor_col, text);

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
    if (buf->cursor_col > 0 && cur_line(buf)->len > 0) {
        line_delete_at(cur_line(buf), buf->cursor_col - 1);

        buf->cursor_col -= 1;
        buf->cursor_prev_col = buf->cursor_col;

    } else if (cur_line(buf)->len < 1 && buf->len > 1) {
        buffer_delete_line(buf, cur_line(buf));

        if (buf->cursor_row > 0) buf->cursor_row -= 1;

        buf->cursor_col = cur_line(buf)->len;
        buf->cursor_prev_col = buf->cursor_col;
    }
}

void buffer_delete_text_under_cursor(Buffer *buf)
{
    if (cur_line(buf)->len > 0 && buf->cursor_col < cur_line(buf)->len)
        line_delete_at(cur_line(buf), buf->cursor_col);
}

void buffer_update_view(Buffer *buf)
{
    buf->scroll_row = buf->cursor_row;
    buf->scroll_col = buf->cursor_col;

    float margin = buf->font_size.y * 0;

    // Cursor is close to bottom
    if ((buf->cursor_row + 1) * buf->font_size.y >= (buf->view.height + buf->view.y) - margin) {
        buf->view.y = buf->scroll_row * buf->font_size.y - (buf->view.height - buf->font_size.y) + margin;
    }

    // Cursor is close to top
    if (buf->cursor_row * buf->font_size.y <= buf->view.y + margin && buf->view.y > 0) {
        buf->view.y = buf->scroll_row * buf->font_size.y - margin;
    }

    // Cursor is close to right
    if ((buf->cursor_col + 1) * buf->font_size.x >= (buf->view.width + buf->view.x)) {
        buf->view.x = buf->scroll_col * buf->font_size.x - (buf->view.width - buf->font_size.x);
    }

    // Cursor is close to left
    if (buf->cursor_col * buf->font_size.x <= buf->view.x && buf->view.x > 0) {
        buf->view.x = buf->scroll_col * buf->font_size.x;
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

    buffer_update_view(buf);
}

void buffer_move_cursor_right(Buffer *buf)
{
    size_t cur_line_len = cur_line(buf)->len;

    if (buf->cursor_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col += 1;
        buf->cursor_prev_col = buf->cursor_col;
    }

    buffer_update_view(buf);
}

void buffer_move_cursor_up(Buffer *buf)
{
    if (buf->cursor_row == 0) {
        buf->cursor_row = 0;
    } else {
        buf->cursor_row -= 1;
    }

    size_t cur_line_len = cur_line(buf)->len;

    if (cur_line_len == 0) {
        buf->cursor_col = 0;
    }

    if (buf->cursor_prev_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col = buf->cursor_prev_col;
    }

    // TODO: This logic needs to be re-worked
    // if (buf->cursor_row * font_size.y <= buf->view.y + 100 && buf->view.y > 0) {
    //     buf->scroll_row -= 1;
    //     buf->view.y = buf->scroll_row * font_size.y;
    // }
    buffer_update_view(buf);
}

void buffer_move_cursor_down(Buffer *buf)
{
    if (buf->cursor_row >= buf->len - 1) {
        buf->cursor_row = buf->len - 1;
    } else {
        buf->cursor_row += 1;
    }

    size_t cur_line_len = cur_line(buf)->len;

    if (cur_line_len == 0) {
        buf->cursor_col = 0;
    }

    if (buf->cursor_prev_col >= cur_line_len) {
        buf->cursor_col = cur_line_len;
    } else {
        buf->cursor_col = buf->cursor_prev_col;
    }

    // TODO: This logic needs to be re-worked
    //  if (buf->cursor_row * font_size.y >= (buf->view.height + buf->view.y) - 100) {
    //      buf->scroll_row += 1;
    //      buf->view.y = buf->scroll_row * font_size.y;
    //  }
    buffer_update_view(buf);
}

void buffer_move_cursor_line_begin(Buffer *buf)
{
    buf->cursor_col = 0;
    buf->cursor_prev_col = buf->cursor_col;
    buffer_update_view(buf);
}

void buffer_move_cursor_line_end(Buffer *buf)
{
    buf->cursor_col = cur_line(buf)->len;
    buf->cursor_prev_col = buf->cursor_col;
    buffer_update_view(buf);
}

void buffer_move_cursor_begin(Buffer *buf)
{
    buf->cursor_row = 0;

    // buf->scroll_row = buf->cursor_row;
    // buf->view.y = buf->scroll_row;
    buffer_update_view(buf);
}

void buffer_move_cursor_end(Buffer *buf)
{
    if (buf->cursor_row == buf->len-1) return;

    buf->cursor_row = buf->len-1;

    // buf->scroll_row = buf->cursor_row;
    // buf->view.y = buf->scroll_row * font_size.y - (buf->view.height - font_size.y);
    buffer_update_view(buf);
}

void buffer_move_cursor_next_word(Buffer *buf)
{
    char current_char = ' ';

    while (cur_line(buf)->text[buf->cursor_col] != ' ' && current_char != '\n') {
        if (buf->cursor_col >= cur_line(buf)->len) {
            buf->cursor_row += 1;
            buf->cursor_col = 0;
            current_char = '\n';
            continue;
        }

        buf->cursor_col += 1;
        current_char = ' ';
    }

    // TODO: I need to organize this...
    while (
        (current_char == ' '
            || (current_char == '\n' && cur_line(buf)->text[buf->cursor_col] == ' ')
        )
        && cur_line(buf)->text[buf->cursor_col+1] == ' '
    ) {
        buf->cursor_col += 1;
        current_char = ' ';
    }

    if (current_char != '\n') buf->cursor_col += 1;
    buf->cursor_prev_col = buf->cursor_col;

    buffer_update_view(buf);
}

void buffer_move_cursor_prev_word(Buffer *buf)
{
    if (buf->cursor_col == 0 && buf->cursor_row == 0) return;

    if (buf->cursor_col == 0) {
        buf->cursor_row -= 1;
        buf->cursor_col = cur_line(buf)->len;
    }

    while (buf->cursor_col > 0 && cur_line(buf)->text[buf->cursor_col-1] == ' ') {
        buf->cursor_col -= 1;
    }

    if (buf->cursor_col == 0 && cur_line(buf)->text[buf->cursor_col] == ' ') {
        buf->cursor_row -= 1;
        buf->cursor_col = cur_line(buf)->len;
    }

    while (buf->cursor_col > 0 && cur_line(buf)->text[buf->cursor_col] == ' ') {
        buf->cursor_col -= 1;
    }

    while (buf->cursor_col > 0 && cur_line(buf)->text[buf->cursor_col-1] != ' ') {
        buf->cursor_col -= 1;
    }

    buf->cursor_prev_col = buf->cursor_col;

    buffer_update_view(buf);
}

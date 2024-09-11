#ifndef BUFFER_H
#define BUFFER_H

#include <raylib.h>

#define LINE_INIT_CAP 128

typedef struct {
    char *text;
    size_t cap;
    size_t len;
} Line;

Line *line_new();
Line *line_from(const char *text);
void line_insert(Line *line, const char *text, size_t col);
void line_delete(Line *line, size_t col);

#define BUF_INIT_CAP 8

typedef struct {
    Line **lines;
    size_t cap;
    size_t len;
    size_t cursor_row;
    size_t cursor_col;
    size_t cursor_prev_col;
    size_t scroll_row;
    size_t scroll_col;
    const char* name;
    Rectangle view;
} Buffer;

Buffer *buffer_new(const char *name);
void buffer_grow(Buffer *buf);
Buffer *buffer_from_file(const char *filename);
void buffer_new_line(Buffer *buf);
void buffer_insert_text(Buffer *buf, const char *text);
void buffer_delete_text(Buffer *buf);
void buffer_move_cursor_left(Buffer *buf);
void buffer_move_cursor_right(Buffer *buf);
void buffer_move_cursor_up(Buffer *buf, Vector2 font_size);
void buffer_move_cursor_down(Buffer *buf, Vector2 font_size);

#endif // BUFFER_H

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
void line_grow(Line *line);
void line_insert_at(Line *line, size_t col, const char *text);
void line_delete_at(Line *line, size_t col);
void line_free(Line *line);

#define BUF_INIT_CAP 4 // Initial number of lines

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
    Vector2 font_size;
} Buffer;

#define cur_line(b) ((b)->lines[(b)->cursor_row])

void buffer_grow(Buffer *buf);

Buffer *buffer_new(const char *name, Vector2 font_size);
Buffer *buffer_from_file(const char *filename, Vector2 font_size);

void buffer_new_line(Buffer *buf);
void buffer_insert_text(Buffer *buf, const char *text);
void buffer_delete_line(Buffer *buf, Line *line);
void buffer_delete_text(Buffer *buf);
void buffer_delete_text_under_cursor(Buffer *buf);
void buffer_update_view(Buffer *buf);
void buffer_move_cursor_left(Buffer *buf);
void buffer_move_cursor_right(Buffer *buf);
void buffer_move_cursor_up(Buffer *buf);
void buffer_move_cursor_down(Buffer *buf);
void buffer_move_cursor_line_begin(Buffer *buf);
void buffer_move_cursor_line_end(Buffer *buf);
void buffer_move_cursor_begin(Buffer *buf);
void buffer_move_cursor_end(Buffer *buf);
void buffer_move_cursor_next_word(Buffer *buf);
void buffer_move_cursor_prev_word(Buffer *buf);

#endif // BUFFER_H

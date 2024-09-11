#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "buffer.h"

#define THEME_BG GetColor(0x0f1115ff)
#define THEME_BG_LIGHT GetColor(0x2c2d31ff)
#define THEME_FG GetColor(0xa7aab0ff)
#define THEME_BLUE GetColor(0x57a5e5ff)

typedef enum {
    NORMAL,
    INSERT
} Mode;

int main(void)
{
    InitWindow(800, 600, "Codigo");
    SetExitKey(0);

    Font font = LoadFontEx(
        "/usr/share/fonts/OTF/DroidSansMNerdFontMono-Regular.otf",
        32, 0, 0
    );

    // Buffer *buf = buffer_new("");
    Buffer *buf = buffer_from_file("main.c");
    Mode mode = NORMAL;

    // float cursor_timer = 0;

    Camera2D camera = {0};
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){0, 0};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Vector2 font_size = MeasureTextEx(font, " ", font.baseSize, 0);

    SetTargetFPS(30);

    while(!WindowShouldClose()) {

        buf->view.width = GetScreenWidth();
        buf->view.height = GetScreenHeight();

        // Just DEBUG stuff
        printf("CURSOR ROW: %ld | ", buf->cursor_row);
        printf("CURSOR COL: %ld | ", buf->cursor_col);

        printf("SCROLL ROW: %ld | ", buf->scroll_row);
        printf("SCROLL COL: %ld | ", buf->scroll_col);

        printf("VIEW Y: %f | ", buf->view.y);
        printf("VIEW HEIGHT: %f | ", buf->view.height);

        printf("CAMERA Y: %f | ", camera.offset.y);
        printf("CAMERA X: %f\n", camera.offset.x);

        if (mode == NORMAL) {
            if (IsKeyPressed(KEY_L)) buffer_move_cursor_right(buf);
            if (IsKeyPressed(KEY_H)) buffer_move_cursor_left(buf);
            if (IsKeyPressed(KEY_J)) buffer_move_cursor_down(buf, font_size);
            if (IsKeyPressed(KEY_K)) buffer_move_cursor_up(buf, font_size);
            if (IsKeyPressed(KEY_I)) mode = INSERT;
        } else {
            if (IsKeyPressed(KEY_RIGHT)) buffer_move_cursor_right(buf);
            if (IsKeyPressed(KEY_LEFT))  buffer_move_cursor_left(buf);
            if (IsKeyPressed(KEY_DOWN))  buffer_move_cursor_down(buf, font_size);
            if (IsKeyPressed(KEY_UP))    buffer_move_cursor_up(buf, font_size);

            if (IsKeyPressed(KEY_BACKSPACE)) buffer_delete_text(buf);
            if (IsKeyPressed(KEY_ENTER))     buffer_new_line(buf);
            if (IsKeyPressed(KEY_ESCAPE))    mode = NORMAL;
        }

        if (mode == INSERT) {
            int key = GetCharPressed();

            while (key > 0) {
                if (key >= 32 && key <= 126) {
                    char s[2];
                    s[0] = (char)key;
                    s[1] = '\0';
                    buffer_insert_text(buf, s);
                }

                key = GetCharPressed();
            }
        }

        Rectangle cursor_rect = {
            font_size.x*buf->cursor_col, buf->cursor_row*font_size.y,
            font_size.x, font_size.y,
        };

        camera.offset.y = -(buf->view.y);

        BeginDrawing();

        ClearBackground(THEME_BG);

        // cursor_timer += GetFrameTime();
        // printf("%f\n", cursor_timer);

        BeginMode2D(camera);

        // RENDER CURSOR
        DrawRectangleRec(cursor_rect, THEME_BLUE);

        float dx = 0;
        float dy = 0;

        // RENDER TEXT
        for (size_t row = 0; row < buf->len; row++) {
            Line *line = buf->lines[row];
            dx = 0;

            for (size_t col = 0; col < line->len; col++) {
                int c = line->text[col];

                char *str = LoadUTF8(&c, 1);

                dy = font.baseSize * row;

                Color text_color = THEME_FG;

                // // Buggy??
                // Color text_color =
                //     col == buf->cursor_col && row == buf->cursor_row
                //     ? THEME_BG : THEME_FG;

                DrawTextEx(
                    font, str,
                    (Vector2){dx, dy},
                    font.baseSize,
                    0, text_color
                );

                UnloadUTF8(str);

                dx += font_size.x;
            }
        }

        EndMode2D();

        Rectangle status_bar_rect = {
            .x = 0,
            .y = GetScreenHeight() - font_size.y,
            .width = GetScreenWidth(),
            .height = font_size.y
        };

        DrawRectangleRec(status_bar_rect, THEME_BG_LIGHT);

        Rectangle status_bar_mode_rect = {
            .x = status_bar_rect.x,
            .y = status_bar_rect.y,
            .width = font_size.x * 5,
            .height = status_bar_rect.height
        };

        DrawRectangleRec(status_bar_mode_rect, DARKGRAY);

        DrawTextEx(
            font, mode == INSERT ? "INS" : "NOR",
            (Vector2){status_bar_rect.x + font_size.x, status_bar_rect.y},
            font.baseSize,
            0, THEME_FG
        );

        DrawTextEx(
            font, buf->name,
            (Vector2){status_bar_rect.x + font_size.x*6, status_bar_rect.y},
            font.baseSize,
            0, THEME_FG
        );

        const char *row_col_text = TextFormat("%ld:%ld", buf->cursor_row + 1, buf->cursor_col + 1);

        DrawTextEx(
            font, row_col_text,
            (Vector2){
                status_bar_rect.width - TextLength(row_col_text)*font_size.x - font_size.x,
                status_bar_rect.y
            },
            font.baseSize,
            0, THEME_FG
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

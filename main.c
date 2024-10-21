#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "buffer.h"

#define THEME_FG GetColor(0xa7aab0ff)
#define THEME_BG GetColor(0x0f1115ff)
#define THEME_BG_1 GetColor(0x2c2d31ff)
#define THEME_BG_2 GetColor(0x35363bff)
#define THEME_BG_3 GetColor(0x37383dff)
#define THEME_GREY GetColor(0x5a5b5eff)
#define THEME_LIGHT_GREY GetColor(0x818387ff)
#define THEME_BLUE GetColor(0x57a5e5ff)

#define FACTOR 72
#define SCR_WIDTH 16 * FACTOR
#define SCR_HEIGHT 9 * FACTOR
#define FONT_SIZE 30
#define TAB_CHARS "    "

typedef enum {
    NORMAL,
    INSERT
} Mode;

int main(int argc, char **argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(SCR_WIDTH, SCR_HEIGHT, "Codigo");

    SetExitKey(0);

    // TODO: Properly include font in project or
    // somehow use the system default mono font
    Font font = LoadFontEx(
        "/usr/share/fonts/OTF/DroidSansMNerdFont-Regular.otf",
        FONT_SIZE, 0, 0
    );

    Vector2 font_size = MeasureTextEx(font, " ", font.baseSize, 0);

    Buffer *buf = NULL;

    if (argc > 1) {
        const char *filename = argv[1];
        // TODO: Check if file exists
        buf = buffer_from_file(filename, font_size);
    } else {
        buf = buffer_new("", font_size);
    }

    Mode mode = NORMAL;

    float key_down_timer = 0;

    Camera2D camera = {0};
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){0, 0};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool ignore_insert_mode_key = false;

    SetTargetFPS(60);

    while(!WindowShouldClose()) {

        buf->view.width = GetScreenWidth();
        buf->view.height = GetScreenHeight() - font_size.y;

        // Just DEBUG stuff
        // printf("CURSOR ROW: %ld | ", buf->cursor_row);
        // printf("CURSOR COL: %ld | ", buf->cursor_col);

        // printf("SCROLL ROW: %ld | ", buf->scroll_row);
        // printf("SCROLL COL: %ld | ", buf->scroll_col);

        // printf("VIEW Y: %f | ", buf->view.y);
        // printf("VIEW HEIGHT: %f | ", buf->view.height);

        // printf("CAMERA Y: %f | ", camera.offset.y);
        // printf("CAMERA X: %f\n", camera.offset.x);

        if (mode == NORMAL) {
            if (IsKeyDown(KEY_H)) {
                key_down_timer += GetFrameTime();

                if (IsKeyPressed(KEY_H)) buffer_move_cursor_left(buf);

                if (key_down_timer >= 0.2) {
                    buffer_move_cursor_left(buf);
                }
            }

            if (IsKeyDown(KEY_L)) {
                key_down_timer += GetFrameTime();

                if (IsKeyPressed(KEY_L)) buffer_move_cursor_right(buf);

                if (key_down_timer >= 0.2) {
                    buffer_move_cursor_right(buf);
                }
            }

            if (IsKeyDown(KEY_J)) {
                key_down_timer += GetFrameTime();

                if (IsKeyPressed(KEY_J)) buffer_move_cursor_down(buf);

                if (key_down_timer >= 0.2) {
                    buffer_move_cursor_down(buf);
                }
            }

            if (IsKeyDown(KEY_K)) {
                key_down_timer += GetFrameTime();

                if (IsKeyPressed(KEY_K)) buffer_move_cursor_up(buf);

                if (key_down_timer >= 0.2) {
                    buffer_move_cursor_up(buf);
                }
            }

            if (IsKeyReleased(KEY_J)) key_down_timer = 0;
            if (IsKeyReleased(KEY_K)) key_down_timer = 0;
            if (IsKeyReleased(KEY_H)) key_down_timer = 0;
            if (IsKeyReleased(KEY_L)) key_down_timer = 0;

            if (IsKeyPressed(KEY_I)) {
                mode = INSERT;
                ignore_insert_mode_key = true;
            }

            if (IsKeyPressed(KEY_X)) buffer_delete_text_under_cursor(buf);
            if (IsKeyPressed(KEY_ZERO)) buffer_move_cursor_line_begin(buf);

            if (IsKeyPressed(KEY_G)) {
                buffer_move_cursor_begin(buf);
            }

            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (IsKeyPressed(KEY_FOUR)) buffer_move_cursor_line_end(buf);
                if (IsKeyPressed(KEY_G)) buffer_move_cursor_end(buf);
                if (IsKeyPressed(KEY_W)) buffer_move_cursor_next_word(buf);
                if (IsKeyPressed(KEY_B)) buffer_move_cursor_prev_word(buf);
            }

            if (IsKeyPressed(KEY_F2)) {
                const char *filename = "output-file";
                buffer_save_to_file(buf,filename);
                printf("File was saved: %s\n", filename);
            }
        } else {
            if (IsKeyPressed(KEY_RIGHT)) buffer_move_cursor_right(buf);
            if (IsKeyPressed(KEY_LEFT))  buffer_move_cursor_left(buf);
            if (IsKeyPressed(KEY_DOWN))  buffer_move_cursor_down(buf);
            if (IsKeyPressed(KEY_UP))    buffer_move_cursor_up(buf);

            if (IsKeyPressed(KEY_BACKSPACE)) buffer_delete_text(buf);
            if (IsKeyPressed(KEY_ENTER))     buffer_new_line(buf);
            if (IsKeyPressed(KEY_TAB))       buffer_insert_text(buf, TAB_CHARS);
            if (IsKeyPressed(KEY_ESCAPE))    {
                mode = NORMAL;
                buffer_move_cursor_left(buf);
            }

            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                if (IsKeyPressed(KEY_C)) {
                    mode = NORMAL;
                    buffer_move_cursor_left(buf);
                }
            }
        }

        if (mode == INSERT) {
            int key = GetCharPressed();

            while (key > 0) {
                if (IsKeyReleased(KEY_I) || ignore_insert_mode_key) {
                    ignore_insert_mode_key = false;
                    key = 0;
                }

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
            mode == INSERT ? font_size.x/6 : font_size.x, font_size.y,
        };

        camera.offset.y = -(buf->view.y);
        camera.offset.x = -(buf->view.x);

        BeginDrawing();

        ClearBackground(THEME_BG);

        BeginMode2D(camera);

        // RENDER CURSOR
        DrawRectangleRec(cursor_rect, THEME_BLUE);

        // RENDER TEXT
        float dx = 0;
        float dy = 0;

        for (size_t row = 0; row < buf->len; row++) {
            Line *line = buf->lines[row];
            dx = 0;

            for (size_t col = 0; col < line->len; col++) {
                int c = line->text[col];

                char *str = LoadUTF8(&c, 1);

                dy = font.baseSize * row;

                // Color text_color = THEME_FG;

                // Buggy?? (Aparently not)
                Color text_color =
                    (col == buf->cursor_col && row == buf->cursor_row) && mode != INSERT
                    ? THEME_BG : THEME_FG;

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

        // RENDER STATUSBAR
        Rectangle status_bar_rect = {
            .x = 0,
            .y = GetScreenHeight() - font_size.y,
            .width = GetScreenWidth(),
            .height = font_size.y
        };

        DrawRectangleRec(status_bar_rect, THEME_BG_2);

        // RENDER STATUSBAR : MODE
        Rectangle status_bar_mode_rect = {
            .x = status_bar_rect.x,
            .y = status_bar_rect.y,
            .width = font_size.x * 5,
            .height = status_bar_rect.height
        };

        DrawRectangleRec(status_bar_mode_rect, mode == INSERT ? THEME_BLUE : THEME_GREY);

        DrawTextEx(
            font, mode == INSERT ? "INS" : "NOR",
            (Vector2){status_bar_rect.x + font_size.x, status_bar_rect.y},
            font.baseSize,
            0, mode == INSERT ? THEME_BG : THEME_FG
        );

        // RENDER STATUSBAR : BUFFER NAME
        DrawTextEx(
            font, buf->name,
            (Vector2){status_bar_rect.x + font_size.x*6, status_bar_rect.y},
            font.baseSize,
            0, THEME_FG
        );

        // RENDER STATUSBAR : COL x ROW
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

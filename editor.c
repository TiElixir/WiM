#include "editor.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal.h"

Editor E;

static char *duplicate_string(const char *source)
{
    size_t length = strlen(source);
    if (length == SIZE_MAX) {
        return NULL;
    }

    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, source, length + 1);
    return copy;
}

static void set_status(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(E.status, sizeof(E.status), format, arguments);
    va_end(arguments);
}

static size_t current_line_length(void)
{
    return buffer_line_length(&E.buffer, E.cy);
}

static void clamp_cursor(void)
{
    size_t line_count = buffer_line_count(&E.buffer);
    if (line_count == 0) {
        E.cy = 0;
        E.cx = 0;
        return;
    }

    if (E.cy >= line_count) {
        E.cy = line_count - 1;
    }

    size_t length = current_line_length();
    if (E.cx > length) {
        E.cx = length;
    }
}

int editorInit(void)
{
    memset(&E, 0, sizeof(E));
    if (buffer_init(&E.buffer) != 0) {
        return -1;
    }

    E.mode = MODE_NORMAL;
    E.screen_rows = 24;
    E.screen_columns = 80;
    set_status("Press i to insert; q to quit.");
    return 0;
}

int editorOpen(const char *filename)
{
    if (filename == NULL || filename[0] == '\0') {
        set_status("A filename is required.");
        return -1;
    }

    char *new_filename = duplicate_string(filename);
    if (new_filename == NULL) {
        set_status("Unable to store filename: out of memory.");
        return -1;
    }

    Buffer loaded;
    if (buffer_init(&loaded) != 0) {
        free(new_filename);
        set_status("Unable to open %s: out of memory.", filename);
        return -1;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        int open_error = errno;
        if (open_error != ENOENT) {
            free(new_filename);
            buffer_free(&loaded);
            set_status("Unable to open %s: %s.", filename, strerror(open_error));
            return -1;
        }
    } else {
        if (buffer_load(&loaded, file) != 0) {
            int read_error = errno;
            fclose(file);
            free(new_filename);
            buffer_free(&loaded);
            set_status("Unable to read %s: %s.", filename, strerror(read_error));
            return -1;
        }
        if (fclose(file) != 0) {
            int close_error = errno;
            free(new_filename);
            buffer_free(&loaded);
            set_status("Unable to close %s: %s.", filename, strerror(close_error));
            return -1;
        }
    }

    buffer_free(&E.buffer);
    E.buffer = loaded;
    free(E.filename);
    E.filename = new_filename;
    E.cx = 0;
    E.cy = 0;
    E.row_offset = 0;
    E.column_offset = 0;
    E.mode = MODE_NORMAL;

    if (file == NULL) {
        set_status("%s: new file.", filename);
    } else {
        set_status("%s: %zu lines.", filename, buffer_line_count(&E.buffer));
    }
    return 0;
}

int editorSave(void)
{
    if (E.filename == NULL || E.filename[0] == '\0') {
        set_status("No filename: open WiM with a path before saving.");
        return -1;
    }

    FILE *file = fopen(E.filename, "w");
    if (file == NULL) {
        set_status("Unable to save %s: %s.", E.filename, strerror(errno));
        return -1;
    }

    if (buffer_save(&E.buffer, file) != 0) {
        int write_error = errno;
        fclose(file);
        set_status("Unable to save %s: %s.", E.filename, strerror(write_error));
        return -1;
    }
    if (fclose(file) != 0) {
        set_status("Unable to save %s: %s.", E.filename, strerror(errno));
        return -1;
    }

    buffer_mark_clean(&E.buffer);
    set_status("%s: %zu lines written.", E.filename, buffer_line_count(&E.buffer));
    return 0;
}

int editorGetLastFilledLine(void)
{
    size_t line_count = buffer_line_count(&E.buffer);
    for (size_t row = line_count; row > 0; --row) {
        const char *line = buffer_line_at(&E.buffer, row - 1);
        if (line != NULL && line[0] != '\0') {
            return row - 1 > INT_MAX ? INT_MAX : (int)(row - 1);
        }
    }
    return -1;
}

static int update_screen_size(void)
{
    int rows;
    int columns;
    if (terminalGetSize(&rows, &columns) != 0) {
        return -1;
    }

    E.screen_rows = rows > 2 ? rows - 2 : 1;
    E.screen_columns = columns > 0 ? columns : 80;
    return 0;
}

static size_t line_number_width(void)
{
    size_t line_count = buffer_line_count(&E.buffer);
    size_t width = 1;
    while (line_count >= 10) {
        line_count /= 10;
        ++width;
    }
    return width < 4 ? 4 : width;
}

static void scroll_to_cursor(void)
{
    size_t visible_rows = E.screen_rows > 0 ? (size_t)E.screen_rows : 1;
    size_t number_width = line_number_width();
    size_t visible_columns = E.screen_columns > (int)(number_width + 1)
        ? (size_t)E.screen_columns - number_width - 1
        : 1;

    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + visible_rows) {
        E.row_offset = E.cy - visible_rows + 1;
    }

    if (E.cx < E.column_offset) {
        E.column_offset = E.cx;
    }
    if (E.cx >= E.column_offset + visible_columns) {
        E.column_offset = E.cx - visible_columns + 1;
    }
}

static void move_vertical(int direction)
{
    size_t line_count = buffer_line_count(&E.buffer);
    if (line_count == 0) {
        return;
    }

    if (direction < 0) {
        if (E.cy > 0) {
            --E.cy;
        }
    } else if (E.cy + 1 < line_count) {
        ++E.cy;
    }
    clamp_cursor();
}

static void move_horizontal(int direction)
{
    size_t length = current_line_length();
    if (direction < 0) {
        if (E.cx > 0) {
            --E.cx;
        }
    } else if (E.cx < length) {
        ++E.cx;
    }
}

static void move_to_start(void)
{
    E.cx = 0;
}

static void move_to_end(void)
{
    E.cx = current_line_length();
}

int editorDisplay(void)
{
    if (update_screen_size() != 0) {
        return -1;
    }
    clamp_cursor();
    scroll_to_cursor();

    printf("\x1b[2J\x1b[H");

    size_t number_width = line_number_width();
    size_t visible_columns = E.screen_columns > (int)(number_width + 1)
        ? (size_t)E.screen_columns - number_width - 1
        : 1;
    size_t line_count = buffer_line_count(&E.buffer);

    for (int screen_row = 0; screen_row < E.screen_rows; ++screen_row) {
        size_t file_row = E.row_offset + (size_t)screen_row;
        fputs("\x1b[K", stdout);

        if (file_row >= line_count) {
            fputs("~", stdout);
        } else {
            const char *line = buffer_line_at(&E.buffer, file_row);
            size_t line_length = buffer_line_length(&E.buffer, file_row);
            size_t start = E.column_offset < line_length ? E.column_offset : line_length;
            size_t display_length = line_length - start;
            if (display_length > visible_columns) {
                display_length = visible_columns;
            }

            printf("%*zu ", (int)number_width, file_row + 1);
            if (display_length > 0) {
                fwrite(line + start, 1, display_length, stdout);
            }
        }
        fputs("\r\n", stdout);
    }

    char status_line[512];
    const char *name = E.filename == NULL ? "[No Name]" : E.filename;
    const char *modified = buffer_is_modified(&E.buffer) ? " [+]" : "";
    int status_length = snprintf(
        status_line,
        sizeof(status_line),
        " %s%s | %s | Ln %zu, Col %zu | %zu lines",
        name,
        modified,
        E.mode == MODE_INSERT ? "INSERT" : "NORMAL",
        E.cy + 1,
        E.cx + 1,
        line_count
    );
    if (status_length < 0) {
        status_length = 0;
    }
    size_t status_bytes = (size_t)status_length;
    if (status_bytes > (size_t)E.screen_columns) {
        status_bytes = (size_t)E.screen_columns;
    }

    fputs("\x1b[7m", stdout);
    fwrite(status_line, 1, status_bytes, stdout);
    fputs("\x1b[K\x1b[27m\r\n", stdout);

    size_t message_bytes = strlen(E.status);
    if (message_bytes > (size_t)E.screen_columns) {
        message_bytes = (size_t)E.screen_columns;
    }
    fwrite(E.status, 1, message_bytes, stdout);
    fputs("\x1b[K", stdout);

    size_t cursor_row = E.cy - E.row_offset + 1;
    size_t cursor_column = number_width + 2 + E.cx - E.column_offset;
    if (cursor_column > (size_t)E.screen_columns) {
        cursor_column = (size_t)E.screen_columns;
    }
    printf("\x1b[%zu;%zuH", cursor_row, cursor_column);
    fflush(stdout);
    return ferror(stdout) ? -1 : 0;
}

static void move_from_key(int key)
{
    switch (key) {
        case 'h':
        case 'a':
        case KEY_ARROW_LEFT:
            move_horizontal(-1);
            break;
        case 'l':
        case 'd':
        case KEY_ARROW_RIGHT:
            move_horizontal(1);
            break;
        case 'k':
        case 'w':
        case KEY_ARROW_UP:
            move_vertical(-1);
            break;
        case 'j':
        case 's':
        case KEY_ARROW_DOWN:
            move_vertical(1);
            break;
        case KEY_HOME:
        case '0':
            move_to_start();
            break;
        case KEY_END:
        case '$':
            move_to_end();
            break;
        default:
            break;
    }
}

static void delete_at_cursor(void)
{
    size_t length = current_line_length();
    if (E.cx < length) {
        if (buffer_delete_char(&E.buffer, E.cy, E.cx + 1) != 0) {
            set_status("Unable to delete character.");
        }
        return;
    }

    if (E.cy + 1 < buffer_line_count(&E.buffer)) {
        if (buffer_join_lines(&E.buffer, E.cy) != 0) {
            set_status("Unable to join lines.");
        }
    }
}

static void backspace_at_cursor(void)
{
    if (E.cx > 0) {
        if (buffer_delete_char(&E.buffer, E.cy, E.cx) == 0) {
            --E.cx;
        } else {
            set_status("Unable to delete character.");
        }
        return;
    }

    if (E.cy == 0) {
        return;
    }

    size_t previous_length = buffer_line_length(&E.buffer, E.cy - 1);
    if (buffer_join_lines(&E.buffer, E.cy - 1) == 0) {
        --E.cy;
        E.cx = previous_length;
    } else {
        set_status("Unable to join lines.");
    }
}

static void insert_mode_key(int key)
{
    if (key == KEY_ESCAPE) {
        E.mode = MODE_NORMAL;
        set_status("Normal mode.");
        return;
    }

    if (key == '\r' || key == '\n') {
        if (buffer_split_line(&E.buffer, E.cy, E.cx) == 0) {
            ++E.cy;
            E.cx = 0;
        } else {
            set_status("Unable to create a new line.");
        }
        return;
    }

    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        backspace_at_cursor();
        return;
    }

    if (key == KEY_DELETE) {
        delete_at_cursor();
        return;
    }

    if (key == KEY_ARROW_LEFT || key == KEY_ARROW_RIGHT ||
        key == KEY_ARROW_UP || key == KEY_ARROW_DOWN ||
        key == KEY_HOME || key == KEY_END) {
        move_from_key(key);
        return;
    }

    if (key == '\t' || (key >= 32 && key <= 126)) {
        if (buffer_insert_char(&E.buffer, E.cy, E.cx, (char)key) == 0) {
            ++E.cx;
        } else {
            set_status("Unable to insert character.");
        }
    }
}

static int normal_mode_key(int key)
{
    switch (key) {
        case 'i':
            E.mode = MODE_INSERT;
            set_status("Insert mode. Escape returns to normal mode.");
            return 0;
        case 'q':
            return 1;
        case 'n':
        case 19:
            editorSave();
            return 0;
        case 'x':
            if (E.cx < current_line_length()) {
                if (buffer_delete_char(&E.buffer, E.cy, E.cx + 1) != 0) {
                    set_status("Unable to delete character.");
                }
            }
            return 0;
        case KEY_DELETE:
            delete_at_cursor();
            return 0;
        default:
            move_from_key(key);
            return 0;
    }
}

int editorRun(void)
{
    if (terminalEnableRawMode() != 0) {
        fprintf(stderr, "WiM: unable to enable raw terminal mode: %s\n", strerror(errno));
        return -1;
    }

    int result = 0;
    for (;;) {
        if (editorDisplay() != 0) {
            result = -1;
            break;
        }

        int key = terminalReadKey();
        if (key == KEY_INTERRUPTED) {
            result = -1;
            break;
        }
        if (key == KEY_ERROR) {
            set_status("Unable to read terminal input.");
            result = -1;
            break;
        }
        if (terminalSignalPending()) {
            result = -1;
            break;
        }

        int should_quit;
        if (E.mode == MODE_INSERT) {
            insert_mode_key(key);
            should_quit = 0;
        } else {
            should_quit = normal_mode_key(key);
        }
        clamp_cursor();
        if (should_quit) {
            break;
        }
    }

    terminalDisableRawMode();
    return result;
}

void editorShutdown(void)
{
    terminalDisableRawMode();
    buffer_free(&E.buffer);
    free(E.filename);
    E.filename = NULL;
}

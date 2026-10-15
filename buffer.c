#include "buffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char *duplicate_range(const char *source, size_t length)
{
    if (length == SIZE_MAX) {
        return NULL;
    }

    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    if (length > 0) {
        memcpy(copy, source, length);
    }
    copy[length] = '\0';
    return copy;
}

static int buffer_reserve_lines(Buffer *buffer, size_t required)
{
    if (required <= buffer->capacity) {
        return 0;
    }

    size_t new_capacity = buffer->capacity == 0 ? 8 : buffer->capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2) {
            new_capacity = required;
            break;
        }
        new_capacity *= 2;
    }

    if (new_capacity > SIZE_MAX / sizeof(*buffer->lines)) {
        return -1;
    }

    char **new_lines = realloc(buffer->lines, new_capacity * sizeof(*new_lines));
    if (new_lines == NULL) {
        return -1;
    }

    buffer->lines = new_lines;
    buffer->capacity = new_capacity;
    return 0;
}

static int buffer_append_line(Buffer *buffer, const char *line, size_t length)
{
    if (buffer_reserve_lines(buffer, buffer->line_count + 1) != 0) {
        return -1;
    }

    char *copy = duplicate_range(line, length);
    if (copy == NULL) {
        return -1;
    }

    buffer->lines[buffer->line_count++] = copy;
    return 0;
}

static int append_character(char **line, size_t *length, size_t *capacity, char character)
{
    if (*length == SIZE_MAX - 1) {
        return -1;
    }

    if (*length == *capacity) {
        size_t new_capacity = *capacity == 0 ? 64 : *capacity;
        while (new_capacity <= *length) {
            if (new_capacity > SIZE_MAX / 2) {
                new_capacity = *length + 1;
                break;
            }
            new_capacity *= 2;
        }

        char *new_line = realloc(*line, new_capacity);
        if (new_line == NULL) {
            return -1;
        }

        *line = new_line;
        *capacity = new_capacity;
    }

    (*line)[(*length)++] = character;
    return 0;
}

int buffer_init(Buffer *buffer)
{
    if (buffer == NULL) {
        return -1;
    }

    memset(buffer, 0, sizeof(*buffer));
    if (buffer_append_line(buffer, "", 0) != 0) {
        buffer_free(buffer);
        return -1;
    }

    return 0;
}

void buffer_free(Buffer *buffer)
{
    if (buffer == NULL) {
        return;
    }

    for (size_t i = 0; i < buffer->line_count; ++i) {
        free(buffer->lines[i]);
    }
    free(buffer->lines);
    memset(buffer, 0, sizeof(*buffer));
}

int buffer_load(Buffer *buffer, FILE *file)
{
    if (buffer == NULL || file == NULL) {
        return -1;
    }

    Buffer loaded;
    if (buffer_init(&loaded) != 0) {
        return -1;
    }
    free(loaded.lines[0]);
    loaded.line_count = 0;

    char *line = NULL;
    size_t length = 0;
    size_t capacity = 0;
    bool saw_character = false;
    bool ended_with_newline = false;
    int character;

    while ((character = fgetc(file)) != EOF) {
        saw_character = true;

        if (character == '\n') {
            if (length > 0 && line[length - 1] == '\r') {
                --length;
            }
            if (buffer_append_line(&loaded, line == NULL ? "" : line, length) != 0) {
                free(line);
                buffer_free(&loaded);
                return -1;
            }
            length = 0;
            ended_with_newline = true;
            continue;
        }

        if (append_character(&line, &length, &capacity, (char)character) != 0) {
            free(line);
            buffer_free(&loaded);
            return -1;
        }
        ended_with_newline = false;
    }

    if (ferror(file)) {
        free(line);
        buffer_free(&loaded);
        return -1;
    }

    if (!saw_character || !ended_with_newline) {
        if (buffer_append_line(&loaded, line == NULL ? "" : line, length) != 0) {
            free(line);
            buffer_free(&loaded);
            return -1;
        }
    }

    free(line);
    loaded.ends_with_newline = ended_with_newline;
    loaded.modified = false;

    buffer_free(buffer);
    *buffer = loaded;
    return 0;
}

int buffer_save(const Buffer *buffer, FILE *file)
{
    if (buffer == NULL || file == NULL || buffer->line_count == 0) {
        return -1;
    }

    for (size_t i = 0; i < buffer->line_count; ++i) {
        if (fputs(buffer->lines[i], file) == EOF) {
            return -1;
        }

        if (i + 1 < buffer->line_count || buffer->ends_with_newline) {
            if (fputc('\n', file) == EOF) {
                return -1;
            }
        }
    }

    return ferror(file) ? -1 : 0;
}

size_t buffer_line_count(const Buffer *buffer)
{
    return buffer == NULL ? 0 : buffer->line_count;
}

const char *buffer_line_at(const Buffer *buffer, size_t row)
{
    if (buffer == NULL || row >= buffer->line_count) {
        return NULL;
    }
    return buffer->lines[row];
}

size_t buffer_line_length(const Buffer *buffer, size_t row)
{
    const char *line = buffer_line_at(buffer, row);
    return line == NULL ? 0 : strlen(line);
}

int buffer_insert_char(Buffer *buffer, size_t row, size_t column, char character)
{
    char *line = (char *)buffer_line_at(buffer, row);
    if (line == NULL) {
        return -1;
    }

    size_t length = strlen(line);
    if (column > length || length > SIZE_MAX - 2) {
        return -1;
    }

    char *resized = realloc(line, length + 2);
    if (resized == NULL) {
        return -1;
    }

    memmove(resized + column + 1, resized + column, length - column + 1);
    resized[column] = character;
    buffer->lines[row] = resized;
    buffer->modified = true;
    return 0;
}

int buffer_delete_char(Buffer *buffer, size_t row, size_t column)
{
    char *line = (char *)buffer_line_at(buffer, row);
    if (line == NULL || column == 0) {
        return -1;
    }

    size_t length = strlen(line);
    if (column > length) {
        return -1;
    }

    memmove(line + column - 1, line + column, length - column + 1);
    char *resized = realloc(line, length);
    if (resized != NULL) {
        buffer->lines[row] = resized;
    }
    buffer->modified = true;
    return 0;
}

int buffer_split_line(Buffer *buffer, size_t row, size_t column)
{
    const char *line = buffer_line_at(buffer, row);
    if (line == NULL) {
        return -1;
    }

    size_t length = strlen(line);
    if (column > length || buffer_reserve_lines(buffer, buffer->line_count + 1) != 0) {
        return -1;
    }

    char *left = duplicate_range(line, column);
    char *right = duplicate_range(line + column, length - column);
    if (left == NULL || right == NULL) {
        free(left);
        free(right);
        return -1;
    }

    free(buffer->lines[row]);
    buffer->lines[row] = left;
    for (size_t i = buffer->line_count; i > row + 1; --i) {
        buffer->lines[i] = buffer->lines[i - 1];
    }
    buffer->lines[row + 1] = right;
    ++buffer->line_count;
    buffer->modified = true;
    return 0;
}

int buffer_join_lines(Buffer *buffer, size_t row)
{
    if (buffer == NULL || row + 1 >= buffer->line_count) {
        return -1;
    }

    char *first = buffer->lines[row];
    char *second = buffer->lines[row + 1];
    size_t first_length = strlen(first);
    size_t second_length = strlen(second);
    if (second_length > SIZE_MAX - first_length - 1) {
        return -1;
    }

    char *joined = realloc(first, first_length + second_length + 1);
    if (joined == NULL) {
        return -1;
    }

    memcpy(joined + first_length, second, second_length + 1);
    free(second);
    buffer->lines[row] = joined;
    for (size_t i = row + 1; i + 1 < buffer->line_count; ++i) {
        buffer->lines[i] = buffer->lines[i + 1];
    }
    --buffer->line_count;
    buffer->modified = true;
    return 0;
}

bool buffer_is_modified(const Buffer *buffer)
{
    return buffer != NULL && buffer->modified;
}

void buffer_mark_clean(Buffer *buffer)
{
    if (buffer != NULL) {
        buffer->modified = false;
    }
}

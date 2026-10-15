#ifndef BUFFER_H
#define BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    char **lines;
    size_t line_count;
    size_t capacity;
    bool modified;
    bool ends_with_newline;
} Buffer;

int buffer_init(Buffer *buffer);
void buffer_free(Buffer *buffer);

int buffer_load(Buffer *buffer, FILE *file);
int buffer_save(const Buffer *buffer, FILE *file);

size_t buffer_line_count(const Buffer *buffer);
const char *buffer_line_at(const Buffer *buffer, size_t row);
size_t buffer_line_length(const Buffer *buffer, size_t row);

int buffer_insert_char(Buffer *buffer, size_t row, size_t column, char character);
int buffer_delete_char(Buffer *buffer, size_t row, size_t column);
int buffer_split_line(Buffer *buffer, size_t row, size_t column);
int buffer_join_lines(Buffer *buffer, size_t row);

bool buffer_is_modified(const Buffer *buffer);
void buffer_mark_clean(Buffer *buffer);

#endif

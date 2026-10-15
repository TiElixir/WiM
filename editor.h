#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>

#include "buffer.h"

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT = 1
} EditorMode;

typedef struct {
    Buffer buffer;
    size_t cx;
    size_t cy;
    size_t row_offset;
    size_t column_offset;
    int screen_rows;
    int screen_columns;
    EditorMode mode;
    char *filename;
    char status[160];
} Editor;

extern Editor E;

int editorInit(void);
int editorOpen(const char *filename);
int editorDisplay(void);
int editorRun(void);
void editorShutdown(void);
int editorGetLastFilledLine(void);
int editorSave(void);

#endif

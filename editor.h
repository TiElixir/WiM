#ifndef EDITOR_H
#define EDITOR_H

typedef struct {
    char **lines;
    int num_lines;
} Editor;

void editorInit();
void editorOpen(const char *filename);
void editorDisplay();
void editorRun();
void editorShutdown();

#endif
#ifndef EDITOR_H
#define EDITOR_H
#include <termios.h>


typedef struct {
    char **lines;
    int num_lines;

    int cx;
    int cy;

    int mode; //view edit, etc...

    char filename[256];
    
    struct termios original;

} Editor;

extern Editor E;

void editorInit();
void editorOpen(const char *filename);
void editorDisplay();
void editorRun();
void editorShutdown();


void editorSave();


#endif
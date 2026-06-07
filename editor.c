#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "terminal.h"

#define BUFFER_L 1024
#define NORMAL 0
#define INSERT 1

Editor E;

void editorInsertChar(char c);
void editorDeleteChar();
void editorInsertNewline();

void editorInit(){
    E.lines=NULL;
    E.num_lines=0;
    E.cx = 0;
    E.cy = 0;
    E.mode = NORMAL;
}

int lineLength(int row){
    return strlen(E.lines[row]);
}

void editorOpen(const char *filename){
    FILE *fp = fopen(filename, "r");

    if(fp==NULL){
        perror("Error: File not dound");
        exit(69);
    }
    strcpy(E.filename, filename);

    char buffer[BUFFER_L];

    while(fgets(buffer, sizeof(buffer),fp)){
        buffer[strcspn(buffer, "\n")] = '\0';
        E.lines = realloc(E.lines,(E.num_lines + 1) * sizeof(char *));
        E.lines[E.num_lines] = malloc(strlen(buffer) + 1); //+1 for next line

        strcpy(E.lines[E.num_lines], buffer);
        E.num_lines++;
    }

    fclose(fp);
}

void editorDisplay(){
    printf("\x1b[2J");
    printf("\x1b[H");
    // ^clears screen

    for (int i = 0; i < E.num_lines; i++){
        printf("%s\r\n", E.lines[i]);
    }

    printf("\n");

    if (E.mode == 0)
        printf("-- NORMAL --");
    else
        printf("-- INSERT --");
    //cursor highlight
    printf("\x1b[%d;%dH",E.cy + 1,E.cx + 1);
    fflush(stdout);
    
}

void editorRun(){
    enableRawMode();

    while (1){
        editorDisplay();
        char c = readKey();
        if (E.mode == 0){
            switch (c)
            {
                case 'i':
                    E.mode = 1;
                    break;

                case 'q':
                    disableRawMode();
                    return;

                case 'w':
                    if (E.cy > 0)
                        E.cy--;
                    break;

                case 's':
                    if (E.cy < E.num_lines - 1)
                        E.cy++;
                    break;

                case 'a':
                    if (E.cx > 0)
                        E.cx--;
                    break;

                case 'd':
                    if (E.cx < lineLength(E.cy))
                        E.cx++;
                    break;
                case 'n':
                    editorSave();
                    break;
            }
        }
        else
        {
            if (c == '\r' || c == '\n'){
                editorInsertNewline();
            }
            else if (c == 27){
                E.mode = 0;
            }
            else if (c == 127 || c == 8)
            {
                editorDeleteChar();
            }
            else
            {
                editorInsertChar(c);
            }
        }
        int len = lineLength(E.cy);

        if (E.cx > len){
            E.cx = len;
        }
    }
}

void editorInsertChar(char c){
    char *line = E.lines[E.cy];
    int len = strlen(line);

    line = realloc(line, len + 2);
    for(int i = len; i >= E.cx; i--){
        line[i + 1] = line[i];
    }

    line[E.cx] = c;
    E.lines[E.cy] = line;
    E.cx++;
}

void editorDeleteChar(){
    if (E.cx == 0){
        if (E.cy == 0)
            return;

        char *prev = E.lines[E.cy - 1];
        char *curr = E.lines[E.cy];

        int prevLen = strlen(prev);
        int currLen = strlen(curr);

        prev = realloc(prev, prevLen + currLen + 1);

        strcpy(prev + prevLen, curr);

        E.lines[E.cy - 1] = prev;

        free(curr);

        for (int i = E.cy; i < E.num_lines - 1; i++){
            E.lines[i] = E.lines[i + 1];
        }

        E.num_lines--;

        E.cy--;
        E.cx = prevLen;

        return;
    }
    char *line = E.lines[E.cy];
    int len = strlen(line);
    for (int i = E.cx - 1; i < len; i++){
        line[i] = line[i + 1];
    }
    E.cx--;
}
void editorInsertNewline(){
    char *line = E.lines[E.cy];

    char *left = malloc(E.cx + 1);
    strncpy(left, line, E.cx);
    left[E.cx] = '\0';

    char *right = malloc(strlen(line + E.cx) + 1);
    strcpy(right, line + E.cx);

    free(E.lines[E.cy]);
    E.lines[E.cy] = left;
    E.lines = realloc(
        E.lines,
        (E.num_lines + 1) * sizeof(char *)
    );

    for (int i = E.num_lines; i > E.cy + 1; i--)
    {
        E.lines[i] = E.lines[i - 1];
    }

    E.lines[E.cy + 1] = right;

    E.num_lines++;

    E.cy++;
    E.cx = 0;
}

void editorShutdown()
{
    for (int i = 0; i < E.num_lines; i++)
    {
        free(E.lines[i]);
    }

    free(E.lines);
}

void editorSave(){
    FILE *fp = fopen(E.filename, "w");

    if (fp == NULL)
    {
        perror("save");
        return;
    }

    for (int i = 0; i < E.num_lines; i++)
    {
        fprintf(fp, "%s\n", E.lines[i]);
    }

    fclose(fp);
}
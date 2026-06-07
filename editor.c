#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"

#define BUFFER_L 1024

Editor E;

void editorInit(){
    E.lines=NULL;
    E.num_lines=0;
    E.cx = 0;
    E.cy = 0;
}

void editorOpen(const char *filename){
    FILE *fp = fopen(filename, "r");

    if(fp==NULL){
        perror("Error: File not dound");
        exit(69);
    }

    char buffer[BUFFER_L];

    while(fgets(buffer, sizeof(buffer),fp)){
        E.lines = realloc(E.lines,(E.num_lines + 1) * sizeof(char *));
        E.lines[E.num_lines] = malloc(strlen(buffer) + 1); //+1 for next line

        strcpy(E.lines[E.num_lines], buffer);
        E.num_lines++;
    }

    fclose(fp);
}

void editorDisplay(){
    for (int i = 0; i < E.num_lines; i++){
        printf("%s", E.lines[i]);
    }
}

void editorRun(){
    editorDisplay();
}

void editorShutdown()
{
    for (int i = 0; i < E.num_lines; i++)
    {
        free(E.lines[i]);
    }

    free(E.lines);
}
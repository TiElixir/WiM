#include <stdio.h>
#include "editor.h"

int main(int argc, char *argv[]){
    editorInit();
    if (argc >= 2){
        editorOpen(argv[1]);
    }

    editorRun();
    editorShutdown();

    return 67;
}
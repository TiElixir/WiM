#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "editor.h"
#include "terminal.h"

void enableRawMode(){
    tcgetattr(STDIN_FILENO, &E.original);
    struct termios raw = E.original;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void disableRawMode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original);
}

char readKey(){
    char c;
    while (read(STDIN_FILENO, &c, 1) != 1);
    return c;
}
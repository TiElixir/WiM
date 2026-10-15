#ifndef TERMINAL_H
#define TERMINAL_H

enum {
    KEY_ERROR = -1,
    KEY_INTERRUPTED = -2,
    KEY_ESCAPE = 27,
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_HOME,
    KEY_END
};

int terminalEnableRawMode(void);
void terminalDisableRawMode(void);
int terminalReadKey(void);
int terminalGetSize(int *rows, int *columns);
int terminalSignalPending(void);

#endif

#include "terminal.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios original_terminal;
static bool raw_mode_enabled;
static volatile sig_atomic_t received_signal;
static struct sigaction previous_actions[4];
static const int handled_signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
static size_t installed_signal_count;

static void handle_signal(int signal_number)
{
    received_signal = signal_number;
}

static void restore_signal_handlers(void)
{
    while (installed_signal_count > 0) {
        --installed_signal_count;
        sigaction(
            handled_signals[installed_signal_count],
            &previous_actions[installed_signal_count],
            NULL
        );
    }
}

int terminalEnableRawMode(void)
{
    if (raw_mode_enabled) {
        return 0;
    }

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        errno = ENOTTY;
        return -1;
    }

    if (tcgetattr(STDIN_FILENO, &original_terminal) == -1) {
        return -1;
    }

    struct termios raw = original_terminal;
    raw.c_iflag &= (tcflag_t) ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return -1;
    }

    received_signal = 0;
    for (size_t i = 0; i < sizeof(handled_signals) / sizeof(handled_signals[0]); ++i) {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = handle_signal;
        sigemptyset(&action.sa_mask);

        if (sigaction(handled_signals[i], &action, &previous_actions[i]) == -1) {
            restore_signal_handlers();
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
            return -1;
        }
        ++installed_signal_count;
    }

    raw_mode_enabled = true;
    return 0;
}

void terminalDisableRawMode(void)
{
    if (!raw_mode_enabled) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
    raw_mode_enabled = false;
    restore_signal_handlers();
}

static int read_byte(unsigned char *character)
{
    for (;;) {
        ssize_t count = read(STDIN_FILENO, character, 1);
        if (count == 1) {
            return 1;
        }
        if (count == 0) {
            return KEY_EOF;
        }
        if (errno == EINTR) {
            return KEY_INTERRUPTED;
        }
        return KEY_ERROR;
    }
}

static int read_escape_sequence(void)
{
    unsigned char first;
    int result = read_byte(&first);
    if (result != 1) {
        return result == KEY_EOF ? KEY_ESCAPE : result;
    }

    if (first == '[' || first == 'O') {
        unsigned char second;
        result = read_byte(&second);
        if (result != 1) {
            return result == KEY_EOF ? KEY_ESCAPE : result;
        }

        if (first == 'O') {
            switch (second) {
                case 'H':
                    return KEY_HOME;
                case 'F':
                    return KEY_END;
                default:
                    return KEY_ESCAPE;
            }
        }

        switch (second) {
            case 'A':
                return KEY_ARROW_UP;
            case 'B':
                return KEY_ARROW_DOWN;
            case 'C':
                return KEY_ARROW_RIGHT;
            case 'D':
                return KEY_ARROW_LEFT;
            case 'H':
                return KEY_HOME;
            case 'F':
                return KEY_END;
            default:
                break;
        }

        if (second >= '0' && second <= '9') {
            unsigned char terminator;
            result = read_byte(&terminator);
            if (result != 1) {
                return result == KEY_EOF ? KEY_ESCAPE : result;
            }
            if (terminator != '~') {
                return KEY_ESCAPE;
            }

            switch (second) {
                case '1':
                case '7':
                    return KEY_HOME;
                case '3':
                    return KEY_DELETE;
                case '4':
                case '8':
                    return KEY_END;
                default:
                    return KEY_ESCAPE;
            }
        }
    }

    return KEY_ESCAPE;
}

int terminalReadKey(void)
{
    for (;;) {
        unsigned char character;
        int result = read_byte(&character);
        if (result != 1) {
            return result;
        }
        if (character == '\x1b') {
            return read_escape_sequence();
        }
        return character;
    }
}

int terminalGetSize(int *rows, int *columns)
{
    if (rows == NULL || columns == NULL) {
        return -1;
    }

    struct winsize window;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == -1 ||
        window.ws_row == 0 || window.ws_col == 0) {
        *rows = 24;
        *columns = 80;
        return 0;
    }

    *rows = window.ws_row;
    *columns = window.ws_col;
    return 0;
}

int terminalSignalPending(void)
{
    return received_signal != 0;
}

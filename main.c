#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"

int main(int argc, char *argv[])
{
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 ||
                      strcmp(argv[1], "-h") == 0)) {
        printf("Usage: %s [file]\n", argv[0]);
        printf("Open a file in the WiM terminal editor.\n");

        return EXIT_SUCCESS;
    }
    if (argc == 2 && (strcmp(argv[1], "--keys") == 0)) {
        printf("Keybinds:\n");
        printf("i           |   Enter insert mode\n");
        printf("q           |   Quit Editor\n");
        printf("x           |   Delete chat at cursor\n");
        printf("h           |   Cursor Left\n");
        printf("j           |   Cursor Down\n");
        printf("k           |   Cursor Up\n");
        printf("l           |   Cursor Right\n");
        printf("n or Ctrl+S |   Write\n\n");
        return EXIT_SUCCESS;
    }

    if (editorInit() != 0) {
        fprintf(stderr, "WiM: unable to initialize the editor\n");
        return EXIT_FAILURE;
    }

    int exit_status = EXIT_SUCCESS;
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    if (argc == 2 && editorOpen(argv[1]) != 0) {
        exit_status = EXIT_FAILURE;
        goto cleanup;
    }

    if (editorRun() != 0) {
        exit_status = EXIT_FAILURE;
    }

cleanup:
    editorShutdown();
    return exit_status;
}

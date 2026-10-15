#include <stdio.h>
#include <stdlib.h>

#include "editor.h"

int main(int argc, char *argv[])
{
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

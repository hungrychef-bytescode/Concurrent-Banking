#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cli_parser.h"

CLIParse parse_cli(int count, char *cmdarg[]) {
    CLIParse cli_input = {0};

    cli_input.tick = 100;

    if (count < 4) {
        fprintf(stderr,
                "Usage: ./bankdb --accounts=FILE --trace=FILE --deadlock=prevention [--tick-ms=N] [--verbose]\n");
        exit(1);
    }

    /* Validate required flags and parse CLI options. */

    for (int i = 1; i < count; i++) {

        if (strncmp(cmdarg[i], "--accounts=", 11) == 0) {
            snprintf(cli_input.accounts, MAX_FILENAME, "%s", cmdarg[i] + 11);
        } else if (strncmp(cmdarg[i], "--trace=", 8) == 0) {
            snprintf(cli_input.trace, MAX_FILENAME, "%s", cmdarg[i] + 8);
        } else if (strncmp(cmdarg[i], "--deadlock=", 11) == 0) {
            snprintf(cli_input.deadlock_mode, 20, "%s", cmdarg[i] + 11);
            if (strcmp(cli_input.deadlock_mode, "prevention") != 0) {
                fprintf(stderr, "Only deadlock prevention is implemented.\n");
                exit(1);
            }
        } else if (strncmp(cmdarg[i], "--tick-ms=", 10) == 0) {
            char *end;
            errno = 0;
            long tick = strtol(cmdarg[i] + 10, &end, 10);

            if (errno != 0 || *end != '\0' || tick <= 0) {
                fprintf(stderr, "Invalid tick value: %s\n", cmdarg[i] + 10);
                exit(1);
            }

            cli_input.tick = (int) tick;
        } else if (strcmp(cmdarg[i], "--verbose")== 0){
            cli_input.verbose = true;
        } else {
            fprintf(stderr, "Unknown argument %s\n", cmdarg[i]);
            exit(1);
        }
    }

    if (cli_input.accounts[0] == '\0' || cli_input.trace[0] == '\0' || cli_input.deadlock_mode[0] == '\0') {
        fprintf(stderr,
                "Missing required arguments. Usage: ./bankdb --accounts=FILE --trace=FILE --deadlock=prevention [--tick-ms=N] [--verbose]\n");
        exit(1);
    }

    return cli_input;
}
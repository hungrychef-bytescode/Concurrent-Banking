#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cli_parser.h"

CLIParse parse_cli(int count, char *cmdarg[]) {
    CLIParse cli_input = {0};

    cli_input.tick = 100;

    //handle edge cases and argcount check

    for (int i = 1; i < count; i++) {

        if (strncmp(cmdarg[i], "--accounts=", 11) == 0) {
            snprintf(cli_input.accounts, MAX_FILENAME, "%s", cmdarg[i] + 11);
        } else if (strncmp(cmdarg[i], "--trace=", 8) == 0) {
            snprintf(cli_input.trace, MAX_FILENAME, "%s", cmdarg[i] + 8);
        } else if (strncmp(cmdarg[i], "--deadlock=", 11) == 0) {
            snprintf(cli_input.deadlock_mode, 20, "%s", cmdarg[i] + 11);
        } else if (strncmp(cmdarg[i], "--tick-ms=", 10) == 0) {
            char *end;
            long tick = strtol(cmdarg[i] + 10, &end, 10);

            if (*end != '\0' || tick <= 0) {
                fprintf(stderr, "invalid tick value \n");
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

    return cli_input;
}
#include <stdio.h>
#include "cli_parser.h"

int main(int argc, char *argv[]) {

    //move this to cli parser func
    if (argc < 4) {
        printf("USE: ./bankdb --accounts= --trace= --deadlock= --tick-ms --verbose");
        return 1;
    }

    printf("argc: %d\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }

    CLIParse bank_command = parse_cli(argc, argv);

    printf("test\n");
    printf("account: %s\n", bank_command.accounts);
    printf("trace: %s\n", bank_command.trace);
    printf("deadlock: %s\n", bank_command.deadlock_mode);
    printf("tick: %d\n", bank_command.tick);
    printf("verbose: %d\n", bank_command.verbose);

    return 0;
}
#include <stdio.h>
#include "cli_parser.h"
#include "accounts_parser.h"

int main(int argc, char *argv[]) {

    printf("argc: %d\n", argc);

    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }

    CLIParse cli = parse_cli(argc, argv);

    printf("test\n");
    printf("account: %s\n", cli.accounts);
    printf("trace: %s\n", cli.trace);
    printf("deadlock: %s\n", cli.deadlock_mode);
    printf("tick: %d\n", cli.tick);
    printf("verbose: %d\n", cli.verbose);

    Account accounts[MAX_ACCOUNTS];

    int count = parse_accounts(cli.accounts, accounts, MAX_ACCOUNTS);
    printf("count: %d\n", count);
    printf("Initial Account Balance\n");

    for (int i = 0; i < count; i++) {
        printf("account id: %d\n", accounts[i].account_id);
        printf("balance in centavos: %d\n", accounts[i].balance_centavos);
    }

   
    return 0;
}
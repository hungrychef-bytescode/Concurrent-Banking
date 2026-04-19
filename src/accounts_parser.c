#include <stdio.h>
#include <stdlib.h>

#include "accounts_parser.h"

int parse_accounts(const char *filename, Bank *bank) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "error opening accounts file: %s\n", filename);
        return -1;
    }

    bank->num_accounts = 0;
    char line[128];

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        if (bank->num_accounts >= MAX_ACCOUNTS) {
            fprintf(stderr, "max accounts reached: %s\n", filename);
            break;
        }

        Account acc = {0};

        if (sscanf(line, "%d %d", &acc.account_id, &acc.balance_centavos) == 2) {
            bank->accounts[bank->num_accounts] = acc;
            bank->num_accounts++;
        }

    }

    fclose(file);


    return bank->num_accounts;
}
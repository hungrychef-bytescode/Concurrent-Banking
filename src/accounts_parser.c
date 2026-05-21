#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accounts_parser.h"

int parse_accounts(const char *filename, Bank *bank) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "error opening accounts file: %s\n", filename);
        return -1;
    }

    bank->num_accounts = 0;
    char line[128];

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        bank->accounts[i].account_id       = -1;
        bank->accounts[i].balance_centavos = 0;
    }

    while (fgets(line, sizeof(line), file)) {
        // Strip line endings
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '#' || line[0] == '\0') continue;

        Account acc = {0};
        int account_id;
        int balance_centavos;

        if (sscanf(line, "%d %d", &account_id, &balance_centavos) == 2) {
            if (account_id < 0 || account_id >= MAX_ACCOUNTS) {
                fprintf(stderr, "invalid account id %d (must be 0..%d)\n", account_id, MAX_ACCOUNTS - 1);
                continue;
            }
            if (balance_centavos < 0) {
                fprintf(stderr, "invalid balance %d for account %d (must be non-negative)\n",
                        balance_centavos, account_id);
                continue;
            }
            if (bank->accounts[account_id].account_id != -1) {
                fprintf(stderr, "duplicate account id %d ignored\n", account_id);
                continue;
            }

            bank->accounts[account_id].account_id       = account_id;
            bank->accounts[account_id].balance_centavos = balance_centavos;
            bank->num_accounts++;
        }
    }

    fclose(file);
    return bank->num_accounts;
}
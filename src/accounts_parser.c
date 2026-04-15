#include <stdio.h>
#include <stdlib.h>

#include "accounts_parser.h"

int parse_accounts(const char *filename, Account accounts[], int max_accounts) {
    FILE *file = fopen(filename, "r");

    //handle edge cases

    int count = 0;
    char line[128];

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        if (count > max_accounts) {
         fprintf(stderr, "warning");
         break;
        }

        Account acc = {0};

        if (sscanf(line, "%d %d", &acc.account_id, &acc.balance_centavos) == 2) {
            accounts[count] = acc;
            count++;
        }

    }

    fclose(file);


    return count;

}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transactions.h"

OpType parse_operation_type(const char *operation) {
    if (strcmp(operation, "DEPOSIT") == 0) {
        return OP_DEPOSIT;
    } else if (strcmp(operation, "WITHDRAW") == 0) {
        return OP_WITHDRAW;
    } else if (strcmp(operation, "TRANSFER") == 0) {
        return OP_TRANSFER;
    } else if (strcmp(operation, "BALANCE") == 0) {
        return OP_BALANCE;
    } else {
        fprintf(stderr, "invalid operation type: %s\n", operation);
        exit(1);
    }
}

int parse_transactions(const char *filename, Transaction *transactions, int max_transactions) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "error opening transactions file: %s\n", filename);
        exit(1);
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < max_transactions) {

        if (line[0] == '#' || line[0] == '\n') continue;

        Transaction *transaction =&transactions[count];
        Operation operation;
        
        char operation_str[32];

        int parsed = sscanf(line, "%s %d %s %d", transaction->tx_id, &transaction->start_tick, operation_str, &operation.account_id);

        operation.type = parse_operation_type(operation_str);
        transaction->num_ops = 0;

        switch (operation.type) {
            case OP_DEPOSIT:
            case OP_WITHDRAW:
                if (sscanf(line, "%*s %*d %*s %*d %d", &operation.amount_centavos) != 1) {
                    fprintf(stderr, "invalid transaction format: %s\n", line);
                    continue;
                }
                operation.target_account = -1;
                break;
            case OP_TRANSFER:
                if (sscanf(line, "%*s %*d %*s %*d %d %d", &operation.amount_centavos, &operation.target_account) != 2) {
                    fprintf(stderr, "invalid transaction format: %s\n", line);
                    continue;
                }
                break;
            case OP_BALANCE:
                operation.amount_centavos = 0;
                operation.target_account = -1;
                break;
        }
        transaction->ops[transaction->num_ops++] = operation;
        count++;
    }

    fclose(file);
    return count;
}
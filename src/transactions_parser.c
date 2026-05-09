#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transactions.h"

OpType parse_operation_type(const char *operation) {
    if (strcmp(operation, "DEPOSIT") == 0)  return OP_DEPOSIT;
    if (strcmp(operation, "WITHDRAW") == 0) return OP_WITHDRAW;
    if (strcmp(operation, "TRANSFER") == 0) return OP_TRANSFER;
    if (strcmp(operation, "BALANCE") == 0)  return OP_BALANCE;
    fprintf(stderr, "invalid operation type: '%s'\n", operation);
    exit(1);
}

// Find existing transaction by tx_id, or return -1
static int find_transaction(Transaction *transactions, int count, const char *tx_id) {
    for (int i = 0; i < count; i++) {
        if (strcmp(transactions[i].tx_id, tx_id) == 0) {
            return i;
        }
    }
    return -1;
}

int parse_transactions(const char *filename, Transaction *transactions, int max_transactions) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "error opening transactions file: %s\n", filename);
        exit(1);
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char tx_id[32];
        int  start_tick;
        char operation_str[32];
        int  account_id;

        if (sscanf(line, "%31s %d %31s %d",
                   tx_id, &start_tick, operation_str, &account_id) != 4) {
            fprintf(stderr, "invalid line: %s\n", line);
            continue;
        }

        // Find existing transaction with same tx_id, or create new one
        int idx = find_transaction(transactions, count, tx_id);
        if (idx == -1) {
            if (count >= max_transactions) {
                fprintf(stderr, "max transactions reached\n");
                break;
            }
            idx = count++;
            strncpy(transactions[idx].tx_id, tx_id, 31);
            transactions[idx].start_tick  = start_tick;
            transactions[idx].num_ops     = 0;
            transactions[idx].actual_start = -1;
            transactions[idx].actual_end   = -1;
            transactions[idx].wait_ticks   = 0;
            transactions[idx].status       = TX_RUNNING;
        }

        // Build the operation
        Operation op = {0};
        op.type       = parse_operation_type(operation_str);
        op.account_id = account_id;

        switch (op.type) {
            case OP_DEPOSIT:
            case OP_WITHDRAW:
                if (sscanf(line, "%*s %*d %*s %*d %d", &op.amount_centavos) != 1) {
                    fprintf(stderr, "invalid format: %s\n", line);
                    continue;
                }
                op.target_account = -1;
                break;
            case OP_TRANSFER:
                if (sscanf(line, "%*s %*d %*s %*d %d %d",
                           &op.target_account, &op.amount_centavos) != 2) {
                    fprintf(stderr, "invalid format: %s\n", line);
                    continue;
                }
                break;
            case OP_BALANCE:
                op.amount_centavos = 0;
                op.target_account  = -1;
                break;
        }

        transactions[idx].ops[transactions[idx].num_ops++] = op;
    }

    fclose(file);
    return count;
}

#include <stdio.h>
#include "cli_parser.h"
#include "accounts_parser.h"
#include "transactions.h"

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

    Bank bank;

    int count = parse_accounts(cli.accounts, &bank);
    printf("count: %d\n", count);
    printf("Initial Account Balance\n");

    for (int i = 0; i < bank.num_accounts; i++) {
        printf("account id: %d\n", bank.accounts[i].account_id);
        printf("balance in centavos: %d\n", bank.accounts[i].balance_centavos);
    }

    Transaction transactions[MAX_TRANSACTIONS];


    int transaction_count = parse_transactions(cli.trace, transactions, MAX_TRANSACTIONS);
    printf("transaction count: %d\n", transaction_count);

    for (int i = 0; i < transaction_count; i++) {
        Transaction *transaction = &transactions[i];
        printf("transaction id: %s\n", transaction->tx_id);
        printf("start tick: %d\n", transaction->start_tick);

        for (int j = 0; j < transaction->num_ops; j++) {
            Operation *op = &transaction->ops[j];
            printf("operation type: %d\n", op->type);
            printf("account id: %d\n", op->account_id);
            printf("amount in centavos: %d\n", op->amount_centavos);
            if (op->type == OP_TRANSFER) {
                printf("target account id: %d\n", op->target_account);
            }
        }
    }
    
    return 0;
}
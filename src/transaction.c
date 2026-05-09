#include <stdio.h>
#include <string.h>
#include "transaction.h"
#include "bank.h"
#include "timer.h"
#include "buffer_pool.h"

void *execute_transaction(void *arg) {
    Transaction *tx = (Transaction *)arg;

    wait_until_tick(tx->start_tick);
    tx->actual_start = global_tick;

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];

        wait_until_tick(op->start_tick);

        load_account(op->account_id);
        if (op->type == OP_TRANSFER) {
            load_account(op->target_account);
        }

        switch (op->type) {

            case OP_DEPOSIT:
                deposit(op->account_id, op->amount_centavos, &tx->wait_ticks);
                printf("  %s [tick %d]: DEPOSIT  account %d amount %d.%02d PHP\n",
                       tx->tx_id, global_tick,
                       op->account_id,
                       op->amount_centavos / 100, op->amount_centavos % 100);
                break;

            case OP_WITHDRAW:
                if (!withdraw(op->account_id, op->amount_centavos, &tx->wait_ticks)) {
                    printf("  %s [tick %d]: WITHDRAW account %d ABORTED (insufficient funds)\n",
                           tx->tx_id, global_tick, op->account_id);
                    unload_account(op->account_id);
                    tx->status     = TX_ABORTED;
                    tx->actual_end = global_tick;
                    return NULL;
                }
                printf("  %s [tick %d]: WITHDRAW account %d amount %d.%02d PHP\n",
                       tx->tx_id, global_tick,
                       op->account_id,
                       op->amount_centavos / 100, op->amount_centavos % 100);
                break;

            case OP_TRANSFER:
                if (!transfer(op->account_id, op->target_account,
                              op->amount_centavos, &tx->wait_ticks)) {
                    printf("  %s [tick %d]: TRANSFER account %d -> %d ABORTED (insufficient funds)\n",
                           tx->tx_id, global_tick,
                           op->account_id, op->target_account);
                    unload_account(op->account_id);
                    unload_account(op->target_account);
                    tx->status     = TX_ABORTED;
                    tx->actual_end = global_tick;
                    return NULL;
                }
                printf("  %s [tick %d]: TRANSFER account %d -> %d amount %d.%02d PHP\n",
                       tx->tx_id, global_tick,
                       op->account_id, op->target_account,
                       op->amount_centavos / 100, op->amount_centavos % 100);
                break;

            case OP_BALANCE: {
                int balance = get_balance(op->account_id);
                printf("  %s [tick %d]: BALANCE  account %d = %d.%02d PHP\n",
                       tx->tx_id, global_tick,
                       op->account_id,
                       balance / 100, balance % 100);
                break;
            }
        }

        unload_account(op->account_id);
        if (op->type == OP_TRANSFER) {
            unload_account(op->target_account);
        }
    }

    tx->actual_end = global_tick;
    tx->status     = TX_COMMITTED;
    return NULL;
}

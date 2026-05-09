#include <stdio.h>
#include "transaction.h"
#include "bank.h"
#include "timer.h"
#include "buffer_pool.h"

void *execute_transaction(void *arg) {
    Transaction *tx = (Transaction *)arg;

    wait_until_tick(tx->start_tick);
    tx->actual_start = global_tick;

    for (int i = 0; i < tx->num_ops; i++) {
        load_account(tx->ops[i].account_id);
        if (tx->ops[i].type == OP_TRANSFER) {
            load_account(tx->ops[i].target_account);
        }
    }

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];
        int tick_before = global_tick;

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
                    tx->status     = TX_ABORTED;
                    tx->actual_end = global_tick;
                    for (int j = 0; j < tx->num_ops; j++) {
                        unload_account(tx->ops[j].account_id);
                        if (tx->ops[j].type == OP_TRANSFER) {
                            unload_account(tx->ops[j].target_account);
                        }
                    }
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
                    tx->status     = TX_ABORTED;
                    tx->actual_end = global_tick;
                    for (int j = 0; j < tx->num_ops; j++) {
                        unload_account(tx->ops[j].account_id);
                        if (tx->ops[j].type == OP_TRANSFER) {
                            unload_account(tx->ops[j].target_account);
                        }
                    }
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

        tx->wait_ticks += (global_tick - tick_before);
    }

    for (int i = 0; i < tx->num_ops; i++) {
        unload_account(tx->ops[i].account_id);
        if (tx->ops[i].type == OP_TRANSFER) {
            unload_account(tx->ops[i].target_account);
        }
    }

    tx->actual_end = global_tick;
    tx->status     = TX_COMMITTED;
    return NULL;
}

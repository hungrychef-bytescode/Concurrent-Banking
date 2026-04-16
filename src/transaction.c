#include "transaction.h"
#include <stdio.h>

void *transaction_thread(void *arg) {
    TxArgs *args = (TxArgs *)arg;

    Transaction *tx = args->tx;

    timer_wait_for_tick(args->timer, tx->start_tick);

    buffer_pool_acquire(args->pool);

    int aborted = 0;

    for (int i = 0; i < tx->num_ops; i++) {
        // YOU plug your parsed operations here
        // Example:
        // if (operation fails) { aborted = 1; break; }
    }

    if (aborted) {
        printf("Transaction %d ABORTED\n", tx->id);
    } else {
        printf("Transaction %d COMMITTED\n", tx->id);
    }

    buffer_pool_release(args->pool);

    return NULL;
}
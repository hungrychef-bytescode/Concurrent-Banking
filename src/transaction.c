#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "transactions.h"
#include "bank.h"
#include "lock_mgr.h"
#include "timer.h"
#include "buffer_pool.h"

static pthread_mutex_t tx_barrier_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  tx_barrier_cv   = PTHREAD_COND_INITIALIZER;
static int             tx_barrier_open = 0;

static pthread_mutex_t event_log_lock = PTHREAD_MUTEX_INITIALIZER;
static int             last_printed_tick = -1;

static void check_pthread(int rc, const char *msg) {
    if (rc != 0) {
        errno = rc;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void log_event(int tick, const char *fmt, ...) {
    check_pthread(pthread_mutex_lock(&event_log_lock), "pthread_mutex_lock(event_log_lock)");

    if (tick != last_printed_tick) {
        if (last_printed_tick != -1) {
            printf("\n");
        }
        printf("Tick %d:\n", tick);
        last_printed_tick = tick;
    }

    printf("  ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    check_pthread(pthread_mutex_unlock(&event_log_lock), "pthread_mutex_unlock(event_log_lock)");
}

void init_transaction_barrier(int total_transactions) {
    (void)total_transactions;
    tx_barrier_open = 0;
}

void release_transaction_barrier(void) {
    check_pthread(pthread_mutex_lock(&tx_barrier_lock), "pthread_mutex_lock(tx_barrier_lock)");
    tx_barrier_open = 1;
    check_pthread(pthread_cond_broadcast(&tx_barrier_cv), "pthread_cond_broadcast(tx_barrier_cv)");
    check_pthread(pthread_mutex_unlock(&tx_barrier_lock), "pthread_mutex_unlock(tx_barrier_lock)");
}

static void wait_transaction_barrier(void) {
    check_pthread(pthread_mutex_lock(&tx_barrier_lock), "pthread_mutex_lock(tx_barrier_lock)");
    while (!tx_barrier_open) {
        check_pthread(pthread_cond_wait(&tx_barrier_cv, &tx_barrier_lock), "pthread_cond_wait(tx_barrier_cv)");
    }
    check_pthread(pthread_mutex_unlock(&tx_barrier_lock), "pthread_mutex_unlock(tx_barrier_lock)");
}

void *execute_transaction(void *arg) {
    Transaction *tx = (Transaction *)arg;

    wait_transaction_barrier();

    // Load all unique accounts required by this transaction before executing.
    // This pins the transaction's working set in the bounded buffer pool
    // until the transaction commits or aborts.
    int unique_accounts[MAX_OP_PER_TX * 2];
    int account_count = 0;

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];
        int account_ids[2] = {op->account_id, op->target_account};
        int limit = (op->type == OP_TRANSFER) ? 2 : 1;

        for (int j = 0; j < limit; j++) {
            int acct = account_ids[j];
            int found = 0;
            for (int k = 0; k < account_count; k++) {
                if (unique_accounts[k] == acct) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                unique_accounts[account_count++] = acct;
            }
        }
    }

    wait_until_tick(tx->start_tick);
    tx->actual_start = get_current_tick();

    for (int i = 0; i < account_count; i++) {
        load_account(unique_accounts[i]);
    }

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];

        wait_until_tick(op->start_tick);
        int op_tick = get_current_tick();

        switch (op->type) {

            case OP_DEPOSIT:
                log_event(op_tick, "%s started: DEPOSIT account %d amount PHP %d.%02d",
                          tx->tx_id, op->account_id,
                          op->amount_centavos / 100, op->amount_centavos % 100);
                deposit(op->account_id, op->amount_centavos, &tx->wait_ticks);
                log_event(get_current_tick(), "%s completed: DEPOSIT successful",
                          tx->tx_id);
                break;

            case OP_WITHDRAW:
                log_event(op_tick, "%s started: WITHDRAW account %d amount PHP %d.%02d",
                          tx->tx_id, op->account_id,
                          op->amount_centavos / 100, op->amount_centavos % 100);
                if (!withdraw(op->account_id, op->amount_centavos, &tx->wait_ticks)) {
                    log_event(get_current_tick(), "%s completed: WITHDRAW aborted (insufficient funds)",
                              tx->tx_id);
                    tx->status     = TX_ABORTED;
                    tx->actual_end = get_current_tick();
                    for (int j = 0; j < account_count; j++) {
                        unload_account(unique_accounts[j]);
                    }
                    return NULL;
                }
                log_event(get_current_tick(), "%s completed: WITHDRAW successful",
                          tx->tx_id);
                break;

            case OP_TRANSFER:
                log_event(op_tick, "%s started: TRANSFER from %d to %d amount PHP %d.%02d",
                          tx->tx_id, op->account_id, op->target_account,
                          op->amount_centavos / 100, op->amount_centavos % 100);
                if (!transfer(tx->tx_id, op->account_id, op->target_account,
                              op->amount_centavos, &tx->wait_ticks)) {
                    log_event(get_current_tick(), "%s completed: TRANSFER aborted",
                              tx->tx_id);
                    tx->status     = TX_ABORTED;
                    tx->actual_end = get_current_tick();
                    for (int j = 0; j < account_count; j++) {
                        unload_account(unique_accounts[j]);
                    }
                    return NULL;
                }
                log_event(get_current_tick(), "%s completed: TRANSFER successful",
                          tx->tx_id);
                break;

            case OP_BALANCE: {
                log_event(op_tick, "%s started: BALANCE account %d",
                          tx->tx_id, op->account_id);
                int balance = get_balance(op->account_id);
                log_event(get_current_tick(), "%s: Account %d balance = PHP %d.%02d",
                          tx->tx_id, op->account_id,
                          balance / 100, balance % 100);
                break;
            }
        }
    }

    tx->actual_end = get_current_tick();
    tx->status     = TX_COMMITTED;

    for (int i = 0; i < account_count; i++) {
        unload_account(unique_accounts[i]);
    }

    if (verbose_flag) {
        printf("  %s [tick %d]: committed\n", tx->tx_id, tx->actual_end);
    }

    return NULL;
}
/*

 * safe money transfer between two accounts in the bank system.
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "lock_mgr.h"
#include "timer.h"

extern void log_event(int tick, const char *fmt, ...);

static void check_pthread(int rc, const char *msg) {
    if (rc != 0) {
        errno = rc;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int transfer(const char *tx_id, int from_id, int to_id,
             int amount_centavos, int *wait_ticks_out) {
    int from_idx = bank_find_account(from_id);
    int to_idx   = bank_find_account(to_id);

    if (from_idx < 0 || to_idx < 0) {
        fprintf(stderr, "transfer: account not found (%d or %d)\n", from_id, to_id);
        return 0;
    }

    if (from_id == to_id) {
        return 1; // Self-transfer is a no-op when account exists
    }

    // Deadlock prevention via consistent lock ordering
    int first_idx  = (from_id < to_id) ? from_idx : to_idx;
    int second_idx = (from_id < to_id) ? to_idx   : from_idx;
    int first_id   = (from_id < to_id) ? from_id : to_id;
    int second_id  = (from_id < to_id) ? to_id   : from_id;

    int tick_before = get_current_tick();

    check_pthread(pthread_rwlock_wrlock(&bank.accounts[first_idx].lock),
                  "pthread_rwlock_wrlock(first)");
    log_event(get_current_tick(), "%s acquired lock on account %d", tx_id, first_id);
    log_event(get_current_tick(), "[DEADLOCK PREVENTION] %s waiting for account %d", tx_id, second_id);

    usleep(120000);  // Simulate processing delay for contention visibility

    check_pthread(pthread_rwlock_wrlock(&bank.accounts[second_idx].lock),
                  "pthread_rwlock_wrlock(second)");
    log_event(get_current_tick(), "%s acquired lock on account %d", tx_id, second_id);
    log_event(get_current_tick(),
              "%s acquired locks for accounts %d and %d (executing transfer %d -> %d, amount %d cents)",
              tx_id, first_id, second_id, from_id, to_id, amount_centavos);

    if (wait_ticks_out) {
        *wait_ticks_out += (get_current_tick() - tick_before);
    }

    usleep(100000);  // Simulate processing delay

    if (bank.accounts[from_idx].balance_centavos < amount_centavos) {
        printf("  %s TRANSFER: insufficient funds, aborting (account %d has %d, need %d)\n",
               tx_id, from_id, bank.accounts[from_idx].balance_centavos, amount_centavos);

        check_pthread(pthread_rwlock_unlock(&bank.accounts[second_idx].lock),
                      "pthread_rwlock_unlock(second)");
        check_pthread(pthread_rwlock_unlock(&bank.accounts[first_idx].lock),
                      "pthread_rwlock_unlock(first)");
        return 0;  // Insufficient funds
    }

    bank.accounts[from_idx].balance_centavos -= amount_centavos;
    bank.accounts[to_idx].balance_centavos   += amount_centavos;

    check_pthread(pthread_rwlock_unlock(&bank.accounts[second_idx].lock),
                  "pthread_rwlock_unlock(second)");
    check_pthread(pthread_rwlock_unlock(&bank.accounts[first_idx].lock),
                  "pthread_rwlock_unlock(first)");

    return 1;
}

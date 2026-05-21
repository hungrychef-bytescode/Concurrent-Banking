#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bank.h"
#include "timer.h"

// The single global bank instance
Bank bank;

static void check_pthread(int rc, const char *msg) {
    if (rc != 0) {
        errno = rc;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

static atomic_int total_deposited  = 0;
static atomic_int total_withdrawn = 0;

void init_bank(Bank *parsed_bank) {
    // Copy account data and initialize pthread synchronization primitives in the
    // global bank instance. This avoids copying initialized pthread objects.
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        bank.accounts[i].account_id       = parsed_bank->accounts[i].account_id;
        bank.accounts[i].balance_centavos = parsed_bank->accounts[i].balance_centavos;
        check_pthread(pthread_rwlock_init(&bank.accounts[i].lock, NULL), "pthread_rwlock_init(account_lock)");
    }
    bank.num_accounts = parsed_bank->num_accounts;
    check_pthread(pthread_mutex_init(&bank.bank_lock, NULL), "pthread_mutex_init(bank_lock)");
}

void destroy_bank(void) {
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_rwlock_destroy(&bank.accounts[i].lock);
    }
    pthread_mutex_destroy(&bank.bank_lock);
}

// Helper: find account index by account_id
// Returns -1 if not found
int bank_find_account(int account_id) {
    if (account_id < 0 || account_id >= MAX_ACCOUNTS) {
        return -1;
    }
    if (bank.accounts[account_id].account_id != account_id) {
        return -1;
    }
    return account_id;
}

Account *bank_get_account(int account_id) {
    int idx = bank_find_account(account_id);
    return (idx < 0) ? NULL : &bank.accounts[idx];
}

int get_balance(int account_id) {
    int idx = bank_find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "get_balance: account %d not found\n", account_id);
        return -1;
    }

    check_pthread(pthread_rwlock_rdlock(&bank.accounts[idx].lock), "pthread_rwlock_rdlock(get_balance)");
    int balance = bank.accounts[idx].balance_centavos;
    check_pthread(pthread_rwlock_unlock(&bank.accounts[idx].lock), "pthread_rwlock_unlock(get_balance)");

    return balance;
}

void deposit(int account_id, int amount_centavos, int *wait_ticks_out) {
    int idx = bank_find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "deposit: account %d not found\n", account_id);
        return;
    }

    int tick_before = get_current_tick();

    check_pthread(pthread_rwlock_wrlock(&bank.accounts[idx].lock), "pthread_rwlock_wrlock(deposit)");
    usleep(100000);  // Simulate processing delay for better tick-based contention visibility

    if (wait_ticks_out) {
        *wait_ticks_out += (get_current_tick() - tick_before);
    }

    bank.accounts[idx].balance_centavos += amount_centavos;
    atomic_fetch_add(&total_deposited, amount_centavos);

    check_pthread(pthread_rwlock_unlock(&bank.accounts[idx].lock), "pthread_rwlock_unlock(deposit)");
}

int withdraw(int account_id, int amount_centavos, int *wait_ticks_out) {
    int idx = bank_find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "withdraw: account %d not found\n", account_id);
        return 0;
    }

    int tick_before = get_current_tick();

    check_pthread(pthread_rwlock_wrlock(&bank.accounts[idx].lock), "pthread_rwlock_wrlock(withdraw)");
    usleep(120000);  // Simulate processing delay for better tick-based contention visibility

    if (wait_ticks_out) {
        *wait_ticks_out += (get_current_tick() - tick_before);
    }

    if (bank.accounts[idx].balance_centavos < amount_centavos) {
        check_pthread(pthread_rwlock_unlock(&bank.accounts[idx].lock), "pthread_rwlock_unlock(withdraw)");
        return 0;  // Insufficient funds
    }

    bank.accounts[idx].balance_centavos -= amount_centavos;
    atomic_fetch_add(&total_withdrawn, amount_centavos);
    check_pthread(pthread_rwlock_unlock(&bank.accounts[idx].lock), "pthread_rwlock_unlock(withdraw)");
    return 1;
}

int total_balance(void) {
    int total = 0;
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        if (bank.accounts[i].account_id == -1) continue;
        check_pthread(pthread_rwlock_rdlock(&bank.accounts[i].lock), "pthread_rwlock_rdlock(total_balance)");
        usleep(5000);  // Simulate processing delay for better concurrency visibility
        total += bank.accounts[i].balance_centavos;
        check_pthread(pthread_rwlock_unlock(&bank.accounts[i].lock), "pthread_rwlock_unlock(total_balance)");
    }
    return total;
}

int net_external_flow(void) {
    return atomic_load(&total_deposited)
         - atomic_load(&total_withdrawn);
}
#include <stdio.h>
#include <string.h>
#include "bank.h"
#include "timer.h"

// The single global bank instance
Bank bank;

void init_bank(Bank *parsed_bank) {
    bank = *parsed_bank;
}

void destroy_bank(void) {
    for (int i = 0; i < bank.num_accounts; i++) {
        pthread_rwlock_destroy(&bank.accounts[i].lock);
    }
    pthread_mutex_destroy(&bank.bank_lock);
}

// Helper: find account index by account_id
// Returns -1 if not found
static int find_account(int account_id) {
    for (int i = 0; i < bank.num_accounts; i++) {
        if (bank.accounts[i].account_id == account_id) {
            return i;
        }
    }
    return -1;
}

int get_balance(int account_id) {
    int idx = find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "get_balance: account %d not found\n", account_id);
        return -1;
    }

    pthread_rwlock_rdlock(&bank.accounts[idx].lock);
    int balance = bank.accounts[idx].balance_centavos;
    pthread_rwlock_unlock(&bank.accounts[idx].lock);

    return balance;
}

void deposit(int account_id, int amount_centavos, int *wait_ticks_out) {
    int idx = find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "deposit: account %d not found\n", account_id);
        return;
    }

    int tick_before = global_tick;

    pthread_rwlock_wrlock(&bank.accounts[idx].lock);

    if (wait_ticks_out) {
        *wait_ticks_out += (global_tick - tick_before);
    }

    bank.accounts[idx].balance_centavos += amount_centavos;

    pthread_rwlock_unlock(&bank.accounts[idx].lock);
}

int withdraw(int account_id, int amount_centavos, int *wait_ticks_out) {
    int idx = find_account(account_id);
    if (idx < 0) {
        fprintf(stderr, "withdraw: account %d not found\n", account_id);
        return 0;
    }

    int tick_before = global_tick;

    pthread_rwlock_wrlock(&bank.accounts[idx].lock);

    if (wait_ticks_out) {
        *wait_ticks_out += (global_tick - tick_before);
    }

    if (bank.accounts[idx].balance_centavos < amount_centavos) {
        pthread_rwlock_unlock(&bank.accounts[idx].lock);
        return 0;  // Insufficient funds
    }

    bank.accounts[idx].balance_centavos -= amount_centavos;
    pthread_rwlock_unlock(&bank.accounts[idx].lock);
    return 1;
}

int transfer(int from_id, int to_id, int amount_centavos, int *wait_ticks_out) {
    int from_idx = find_account(from_id);
    int to_idx   = find_account(to_id);

    if (from_idx < 0 || to_idx < 0) {
        fprintf(stderr, "transfer: account not found (%d or %d)\n", from_id, to_id);
        return 0;
    }

    // ── Deadlock Prevention via Lock Ordering ──────────────────────────
    // Always acquire locks in ascending order of account_id.
    // This breaks the "circular wait" Coffman condition:
    // no two threads can wait for each other because they always
    // request locks in the same global order.
    int first_idx  = (from_id < to_id) ? from_idx : to_idx;
    int second_idx = (from_id < to_id) ? to_idx   : from_idx;

    int tick_before = global_tick;

    pthread_rwlock_wrlock(&bank.accounts[first_idx].lock);
    pthread_rwlock_wrlock(&bank.accounts[second_idx].lock);

    if (wait_ticks_out) {
        *wait_ticks_out += (global_tick - tick_before);
    }

    if (bank.accounts[from_idx].balance_centavos < amount_centavos) {
        pthread_rwlock_unlock(&bank.accounts[second_idx].lock);
        pthread_rwlock_unlock(&bank.accounts[first_idx].lock);
        return 0;  // Insufficient funds
    }

    bank.accounts[from_idx].balance_centavos -= amount_centavos;
    bank.accounts[to_idx].balance_centavos   += amount_centavos;

    // Always unlock in reverse order of acquisition
    pthread_rwlock_unlock(&bank.accounts[second_idx].lock);
    pthread_rwlock_unlock(&bank.accounts[first_idx].lock);

    return 1;
}

int total_balance(void) {
    int total = 0;
    for (int i = 0; i < bank.num_accounts; i++) {
        pthread_rwlock_rdlock(&bank.accounts[i].lock);
        total += bank.accounts[i].balance_centavos;
        pthread_rwlock_unlock(&bank.accounts[i].lock);
    }
    return total;
}
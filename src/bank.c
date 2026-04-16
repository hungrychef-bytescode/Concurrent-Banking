#include "bank.h"
#include <stdlib.h>

void bank_init(Bank *bank, int num_accounts, long initial_balance) {
    bank->num_accounts = num_accounts;
    bank->accounts = malloc(sizeof(BankAccount) * num_accounts);

    for (int i = 0; i < num_accounts; i++) {
        bank->accounts[i].id = i;
        bank->accounts[i].balance = initial_balance;
        pthread_rwlock_init(&bank->accounts[i].lock, NULL);
    }
}

void bank_destroy(Bank *bank) {
    for (int i = 0; i < bank->num_accounts; i++) {
        pthread_rwlock_destroy(&bank->accounts[i].lock);
    }
    free(bank->accounts);
}

int bank_deposit(Bank *bank, int acc_id, long amount) {
    BankAccount *acc = &bank->accounts[acc_id];

    pthread_rwlock_wrlock(&acc->lock);
    acc->balance += amount;
    pthread_rwlock_unlock(&acc->lock);

    return 0;
}

int bank_withdraw(Bank *bank, int acc_id, long amount) {
    BankAccount *acc = &bank->accounts[acc_id];

    pthread_rwlock_wrlock(&acc->lock);

    if (acc->balance < amount) {
        pthread_rwlock_unlock(&acc->lock);
        return -1;
    }

    acc->balance -= amount;
    pthread_rwlock_unlock(&acc->lock);

    return 0;
}

int bank_transfer(Bank *bank, int from, int to, long amount) {
    if (from == to) return 0;

    int first = from < to ? from : to;
    int second = from < to ? to : from;

    BankAccount *acc1 = &bank->accounts[first];
    BankAccount *acc2 = &bank->accounts[second];

    pthread_rwlock_wrlock(&acc1->lock);
    pthread_rwlock_wrlock(&acc2->lock);

    BankAccount *src = &bank->accounts[from];
    BankAccount *dst = &bank->accounts[to];

    if (src->balance < amount) {
        pthread_rwlock_unlock(&acc2->lock);
        pthread_rwlock_unlock(&acc1->lock);
        return -1;
    }

    src->balance -= amount;
    dst->balance += amount;

    pthread_rwlock_unlock(&acc2->lock);
    pthread_rwlock_unlock(&acc1->lock);

    return 0;
}

long bank_balance(Bank *bank, int acc_id) {
    BankAccount *acc = &bank->accounts[acc_id];

    pthread_rwlock_rdlock(&acc->lock);
    long bal = acc->balance;
    pthread_rwlock_unlock(&acc->lock);

    return bal;
}
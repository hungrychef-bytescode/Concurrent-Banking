#ifndef BANK_H
#define BANK_H

#include <pthread.h>

typedef struct {
    int id;
    long balance;
    pthread_rwlock_t lock;
} BankAccount;

typedef struct {
    BankAccount *accounts;
    int num_accounts;
} Bank;

void bank_init(Bank *bank, int num_accounts, long initial_balance);
void bank_destroy(Bank *bank);

int bank_deposit(Bank *bank, int acc_id, long amount);
int bank_withdraw(Bank *bank, int acc_id, long amount);
int bank_transfer(Bank *bank, int from, int to, long amount);
long bank_balance(Bank *bank, int acc_id);

#endif
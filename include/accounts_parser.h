#ifndef ACCOUNTS_PARSER_H
#define ACCOUNTS_PARSER_H

#include <pthread.h>

#define MAX_ACCOUNTS 100

typedef struct {
    int account_id;
    int balance_centavos;
    pthread_rwlock_t lock;   // Per-account reader-writer lock
} Account;

typedef struct {
    Account          accounts[MAX_ACCOUNTS];
    int              num_accounts;
    pthread_mutex_t  bank_lock;  // Protects bank metadata
} Bank;

int parse_accounts(const char *file, Bank *bank);

#endif
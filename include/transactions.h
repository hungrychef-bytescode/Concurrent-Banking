#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <pthread.h>
#include <stdatomic.h>

#define MAX_OP_PER_TX    256
#define MAX_TRANSACTIONS 100

typedef enum {
    OP_DEPOSIT,
    OP_WITHDRAW,
    OP_TRANSFER,
    OP_BALANCE,
} OpType;

typedef struct {
    OpType type;
    int account_id;
    int amount_centavos;
    int target_account;
    int start_tick;
} Operation;

typedef enum {
    TX_RUNNING,
    TX_COMMITTED,
    TX_ABORTED
} TxStatus;

typedef struct {
    char       tx_id[32];
    Operation  ops[MAX_OP_PER_TX];
    int        num_ops;
    int        start_tick;
    pthread_t  thread;
    int        actual_start;
    int        actual_end;
    int        wait_ticks;
    atomic_int status;
} Transaction;

extern int verbose_flag;

void init_transaction_barrier(int total_transactions);
void release_transaction_barrier(void);

// Entry point for each transaction pthread
// Pass a pointer to a Transaction struct
void *execute_transaction(void *arg);

int parse_transactions(const char *filename, Transaction *transactions, int max_transactions);

#endif

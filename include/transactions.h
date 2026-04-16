#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#define MAX_OP_PER_TX 256
#define MAX_TRANSACTIONS 100

typedef enum {
    OP_DEPOSIT,   // Add money to account
    OP_WITHDRAW,  // Remove money from account
    OP_TRANSFER,  // Move money between two accounts
    OP_BALANCE,   // Read account balance
} OpType;

typedef struct {
    OpType type;
    int account_id;          // Primary account
    int amount_centavos;     // Amount in centavos
    int target_account;      // For TRANSFER only
} Operation;

typedef enum {
    TX_RUNNING,
    TX_COMMITTED,
    TX_ABORTED
} TxStatus;

typedef struct {
    char tx_id[32];
    Operation ops[MAX_OP_PER_TX];    // Max 256 operations per transaction
    int num_ops;
    int start_tick;       // When transaction should start
    // pthread_t thread;
    
    // Timing (measured in ticks)
    // int actual_start;
    // int actual_end;
    // int wait_ticks;
    
    // // Status
    // TxStatus status;
} Transaction;

int parse_transactions(const char *filename, Transaction *transactions, int max_transactions);

#endif
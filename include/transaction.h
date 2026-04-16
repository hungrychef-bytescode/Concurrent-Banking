#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "bank.h"
#include "timer.h"
#include "buffer_pool.h"

typedef struct {
    int id;
    int start_tick;
    int num_ops;
    // assume operations already parsed into array
} Transaction;

typedef struct {
    Transaction *tx;
    Bank *bank;
    Timer *timer;
    BufferPool *pool;
} TxArgs;

void *transaction_thread(void *arg);

#endif
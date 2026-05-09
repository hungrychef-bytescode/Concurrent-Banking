#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "transactions.h"

// Entry point for each transaction pthread
// Pass a pointer to a Transaction struct
void *execute_transaction(void *arg);

#endif
#ifndef LOCK_MGR_H
#define LOCK_MGR_H

#include "bank.h"

// Transfer money between accounts while preventing deadlocks.
// Returns 1 on success, 0 on insufficient funds or missing accounts.
int transfer(const char *tx_id, int from_id, int to_id, int amount_centavos, int *wait_ticks_out);

#endif
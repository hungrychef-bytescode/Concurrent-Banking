#ifndef BANK_H
#define BANK_H

#include "accounts_parser.h"

// The global bank instance (defined in bank.c, used everywhere)
extern Bank bank;

// Initialize the bank from a parsed Bank struct
void init_bank(Bank *parsed_bank);

// Destroy all locks
void destroy_bank(void);

// Banking operations
// Returns current balance of account_id
int  get_balance(int account_id);

// Adds amount to account
void deposit(int account_id, int amount_centavos, int *wait_ticks_out);

// Removes amount from account; returns 1 on success, 0 if insufficient funds
int  withdraw(int account_id, int amount_centavos, int *wait_ticks_out);

// Finds an account by its account_id and returns its index, or -1 if missing.
int  bank_find_account(int account_id);

// Returns a pointer to an account by its account_id, or NULL if missing.
Account *bank_get_account(int account_id);

// Returns the sum of all account balances (for conservation check)
int  total_balance(void);

// Returns external cash flow from deposits and withdrawals
int  net_external_flow(void);

#endif
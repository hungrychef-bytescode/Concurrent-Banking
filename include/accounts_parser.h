#ifndef ACCOUNTS_PARSER_H
#define ACCOUNTS_PARSER_H

#define MAX_ACCOUNTS 100

typedef struct {
    int account_id;
    int balance_centavos;
} Account;

typedef struct P{
    Account accounts[MAX_ACCOUNTS];
    int num_accounts;
} Bank;

int parse_accounts(const char *file, Bank *bank);
#endif
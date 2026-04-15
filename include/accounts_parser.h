#ifndef ACCOUNTS_PARSER_H
#define ACCOUNTS_PARSER_H

#define MAX_ACCOUNTS 100

typedef struct {
    int account_id;
    int balance_centavos;
} Account;

int parse_accounts(const char *file, Account accounts[], int max_accounts);
#endif
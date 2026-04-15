#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <stdbool.h> 

#define MAX_FILENAME 256

typedef struct {
    char accounts[MAX_FILENAME];
    char trace[MAX_FILENAME];
    char deadlock_mode[20];
    int tick;
    bool verbose;
} CLIParse;

CLIParse parse_cli(int count, char *cmdarg[]);

#endif
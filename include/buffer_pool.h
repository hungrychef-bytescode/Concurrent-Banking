#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include "accounts_parser.h"

#define BUFFER_POOL_SIZE 5

typedef struct {
    int      account_id;
    Account *data;
    bool     in_use;
    int      ref_count;
} BufferSlot;

typedef struct {
    BufferSlot      slots[BUFFER_POOL_SIZE];
    sem_t           empty_slots;  // counts available empty slots
    pthread_mutex_t pool_lock;    // protects slots array

    // Statistics
    int total_loads;
    int total_unloads;
    int peak_usage;
    int current_usage;
    int blocked_operations;
} BufferPool;

// Global buffer pool instance
extern BufferPool buffer_pool;

void init_buffer_pool(void);
void destroy_buffer_pool(void);

// Load account into a buffer slot (blocks if pool is full)
void load_account(int account_id);

// Unload account from buffer slot (signals empty slot)
void unload_account(int account_id);

void print_buffer_pool_stats(void);

#endif
#include <stdio.h>
#include "buffer_pool.h"
#include "bank.h"

// Global buffer pool instance
BufferPool buffer_pool;

void init_buffer_pool(void) {
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        buffer_pool.slots[i].in_use     = false;
        buffer_pool.slots[i].account_id = -1;
        buffer_pool.slots[i].data       = NULL;
    }

    // empty_slots starts at BUFFER_POOL_SIZE (all slots are empty)
    sem_init(&buffer_pool.empty_slots, 0, BUFFER_POOL_SIZE);
    // full_slots starts at 0 (no slots are filled)
    sem_init(&buffer_pool.full_slots,  0, 0);

    pthread_mutex_init(&buffer_pool.pool_lock, NULL);

    buffer_pool.total_loads        = 0;
    buffer_pool.total_unloads      = 0;
    buffer_pool.peak_usage         = 0;
    buffer_pool.current_usage      = 0;
    buffer_pool.blocked_operations = 0;
}

void destroy_buffer_pool(void) {
    sem_destroy(&buffer_pool.empty_slots);
    sem_destroy(&buffer_pool.full_slots);
    pthread_mutex_destroy(&buffer_pool.pool_lock);
}

// Producer: load an account into the buffer pool
// Blocks if all slots are full (sem_wait on empty_slots)
void load_account(int account_id) {
    // Check if already loaded — avoid double-loading
    pthread_mutex_lock(&buffer_pool.pool_lock);
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            pthread_mutex_unlock(&buffer_pool.pool_lock);
            return;  // Already in pool
        }
    }
    pthread_mutex_unlock(&buffer_pool.pool_lock);

    // Wait for an empty slot (blocks if pool is full)
    int val;
    sem_getvalue(&buffer_pool.empty_slots, &val);
    if (val == 0) buffer_pool.blocked_operations++;

    sem_wait(&buffer_pool.empty_slots);

    pthread_mutex_lock(&buffer_pool.pool_lock);

    // Find the empty slot and fill it
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (!buffer_pool.slots[i].in_use) {
            // Find the account in the bank
            for (int j = 0; j < bank.num_accounts; j++) {
                if (bank.accounts[j].account_id == account_id) {
                    buffer_pool.slots[i].account_id = account_id;
                    buffer_pool.slots[i].data       = &bank.accounts[j];
                    buffer_pool.slots[i].in_use     = true;
                    break;
                }
            }
            break;
        }
    }

    buffer_pool.total_loads++;
    buffer_pool.current_usage++;
    if (buffer_pool.current_usage > buffer_pool.peak_usage) {
        buffer_pool.peak_usage = buffer_pool.current_usage;
    }

    pthread_mutex_unlock(&buffer_pool.pool_lock);

    // Signal that a full slot is now available
    sem_post(&buffer_pool.full_slots);
}

// Consumer: unload an account from the buffer pool
// Blocks if the account isn't loaded yet (sem_wait on full_slots)
void unload_account(int account_id) {
    sem_wait(&buffer_pool.full_slots);

    pthread_mutex_lock(&buffer_pool.pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            buffer_pool.slots[i].in_use     = false;
            buffer_pool.slots[i].account_id = -1;
            buffer_pool.slots[i].data       = NULL;
            break;
        }
    }

    buffer_pool.total_unloads++;
    buffer_pool.current_usage--;

    pthread_mutex_unlock(&buffer_pool.pool_lock);

    // Signal that an empty slot is now available
    sem_post(&buffer_pool.empty_slots);
}

void print_buffer_pool_stats(void) {
    printf("\n=== Buffer Pool Report ===\n");
    printf("Pool size:                  %d slots\n", BUFFER_POOL_SIZE);
    printf("Total loads:                %d\n", buffer_pool.total_loads);
    printf("Total unloads:              %d\n", buffer_pool.total_unloads);
    printf("Peak usage:                 %d slots\n", buffer_pool.peak_usage);
    printf("Blocked operations:         %d\n", buffer_pool.blocked_operations);
}
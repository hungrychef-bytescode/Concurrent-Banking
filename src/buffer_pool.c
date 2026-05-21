#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "buffer_pool.h"
#include "bank.h"

// Global buffer pool instance
BufferPool buffer_pool;

static void check_errno(int rv, const char *msg) {
    if (rv != 0) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

static void safe_sem_wait(sem_t *sem, const char *msg) {
    while (sem_wait(sem) != 0) {
        if (errno == EINTR) continue;
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void init_buffer_pool(void) {
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        buffer_pool.slots[i].in_use     = false;
        buffer_pool.slots[i].account_id = -1;
        buffer_pool.slots[i].data       = NULL;
        buffer_pool.slots[i].ref_count  = 0;
    }

    // empty_slots starts at BUFFER_POOL_SIZE (all slots are empty)
    check_errno(sem_init(&buffer_pool.empty_slots, 0, BUFFER_POOL_SIZE), "sem_init(empty_slots)");
    check_errno(pthread_mutex_init(&buffer_pool.pool_lock, NULL), "pthread_mutex_init(pool_lock)");

    buffer_pool.total_loads        = 0;
    buffer_pool.total_unloads      = 0;
    buffer_pool.peak_usage         = 0;
    buffer_pool.current_usage      = 0;
    buffer_pool.blocked_operations = 0;
}

void destroy_buffer_pool(void) {
    sem_destroy(&buffer_pool.empty_slots);
    pthread_mutex_destroy(&buffer_pool.pool_lock);
}

// Producer: load an account into the buffer pool
// Blocks if all slots are full (sem_wait on empty_slots)
void load_account(int account_id) {
    // Check if already loaded — avoid double-loading
    check_errno(pthread_mutex_lock(&buffer_pool.pool_lock), "pthread_mutex_lock(pool_lock)");
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            buffer_pool.slots[i].ref_count++;
            check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
            return;  // Already loaded, just increment reference count
        }
    }
    check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");

    // Try to acquire an empty slot without blocking first.
    if (sem_trywait(&buffer_pool.empty_slots) != 0) {
        if (errno != EAGAIN) {
            perror("sem_trywait(empty_slots)");
            exit(EXIT_FAILURE);
        }
        check_errno(pthread_mutex_lock(&buffer_pool.pool_lock), "pthread_mutex_lock(pool_lock)");
        buffer_pool.blocked_operations++;
        check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
        safe_sem_wait(&buffer_pool.empty_slots, "sem_wait(empty_slots)");
    }

    check_errno(pthread_mutex_lock(&buffer_pool.pool_lock), "pthread_mutex_lock(pool_lock)");

    // If the account was loaded while we were waiting, reuse the slot.
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            buffer_pool.slots[i].ref_count++;
            check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
            check_errno(sem_post(&buffer_pool.empty_slots), "sem_post(empty_slots)");
            return;
        }
    }

    // Find the empty slot and fill it
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (!buffer_pool.slots[i].in_use) {
            Account *acct = bank_get_account(account_id);
            if (acct == NULL) {
                fprintf(stderr, "load_account: account %d not found\n", account_id);
                exit(EXIT_FAILURE);
            }
            buffer_pool.slots[i].account_id = account_id;
            buffer_pool.slots[i].data       = acct;
            buffer_pool.slots[i].in_use     = true;
            buffer_pool.slots[i].ref_count  = 1;
            break;
        }
    }

    buffer_pool.total_loads++;
    buffer_pool.current_usage++;
    if (buffer_pool.current_usage > buffer_pool.peak_usage) {
        buffer_pool.peak_usage = buffer_pool.current_usage;
    }

    check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");

    // Simulate buffer load latency so concurrent buffer pool contention is observable.
    usleep(100000);
    return;
}

// Consumer: unload an account from the buffer pool
void unload_account(int account_id) {
    check_errno(pthread_mutex_lock(&buffer_pool.pool_lock), "pthread_mutex_lock(pool_lock)");

    bool found = false;
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (buffer_pool.slots[i].in_use &&
            buffer_pool.slots[i].account_id == account_id) {
            found = true;
            buffer_pool.slots[i].ref_count--;
            if (buffer_pool.slots[i].ref_count <= 0) {
                buffer_pool.slots[i].in_use     = false;
                buffer_pool.slots[i].account_id = -1;
                buffer_pool.slots[i].data       = NULL;
                buffer_pool.total_unloads++;
                buffer_pool.current_usage--;

                check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
                check_errno(sem_post(&buffer_pool.empty_slots), "sem_post(empty_slots)");
                return;
            }

            check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
            return;
        }
    }

    check_errno(pthread_mutex_unlock(&buffer_pool.pool_lock), "pthread_mutex_unlock(pool_lock)");
    if (!found) {
        fprintf(stderr, "unload_account: account %d not found in buffer pool\n", account_id);
    }
}

void print_buffer_pool_stats(void) {
    printf("\n=== Buffer Pool Report ===\n");
    printf("Pool size:                  %d slots\n", BUFFER_POOL_SIZE);
    printf("Total loads:                %d\n", buffer_pool.total_loads);
    printf("Total unloads:              %d\n", buffer_pool.total_unloads);
    printf("Peak usage:                 %d slots\n", buffer_pool.peak_usage);
    printf("Blocked operations:         %d\n", buffer_pool.blocked_operations);
}
#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <semaphore.h>

#define BUFFER_POOL_SIZE 5

typedef struct {
    sem_t slots;
    int blocked_count;
} BufferPool;

void buffer_pool_init(BufferPool *bp);
void buffer_pool_destroy(BufferPool *bp);

void buffer_pool_acquire(BufferPool *bp);
void buffer_pool_release(BufferPool *bp);

#endif
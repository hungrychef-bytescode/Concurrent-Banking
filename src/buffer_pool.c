#include "buffer_pool.h"

void buffer_pool_init(BufferPool *bp) {
    sem_init(&bp->slots, 0, BUFFER_POOL_SIZE);
    bp->blocked_count = 0;
}

void buffer_pool_destroy(BufferPool *bp) {
    sem_destroy(&bp->slots);
}

void buffer_pool_acquire(BufferPool *bp) {
    if (sem_trywait(&bp->slots) != 0) {
        bp->blocked_count++;
        sem_wait(&bp->slots);
    }
}

void buffer_pool_release(BufferPool *bp) {
    sem_post(&bp->slots);
}
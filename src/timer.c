#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include "timer.h"

// Global definitions (declared extern in timer.h)
atomic_int         global_tick       = 0;
atomic_int         simulation_running = 1;
pthread_mutex_t    tick_lock         = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t     tick_changed      = PTHREAD_COND_INITIALIZER;

static pthread_t timer_tid;
static int       tick_interval_ms_global;

// The timer thread: increments global_tick every tick_interval_ms
static void* timer_thread(void *arg) {
    (void)arg;

    while (1) {
        usleep(tick_interval_ms_global * 1000);  // sleep one tick

        int rc = pthread_mutex_lock(&tick_lock);
        if (rc != 0) {
            errno = rc;
            perror("pthread_mutex_lock");
            exit(EXIT_FAILURE);
        }

        if (!atomic_load(&simulation_running)) {
            rc = pthread_mutex_unlock(&tick_lock);
            if (rc != 0) {
                errno = rc;
                perror("pthread_mutex_unlock");
                exit(EXIT_FAILURE);
            }
            break;
        }

        atomic_fetch_add(&global_tick, 1);

        rc = pthread_cond_broadcast(&tick_changed);   // wake all waiting threads
        if (rc != 0) {
            errno = rc;
            perror("pthread_cond_broadcast");
            pthread_mutex_unlock(&tick_lock);
            exit(EXIT_FAILURE);
        }

        rc = pthread_mutex_unlock(&tick_lock);
        if (rc != 0) {
            errno = rc;
            perror("pthread_mutex_unlock");
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}

// Start the timer thread
void init_timer(int tick_interval_ms) {
    tick_interval_ms_global = tick_interval_ms;
    int rc = pthread_create(&timer_tid, NULL, timer_thread, NULL);
    if (rc != 0) {
        errno = rc;
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
}

// Return the current global tick in a thread-safe way
int get_current_tick(void) {
    int tick;
    int rc = pthread_mutex_lock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_lock");
        exit(EXIT_FAILURE);
    }
    tick = atomic_load(&global_tick);
    rc = pthread_mutex_unlock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
    return tick;
}

// Block until the global clock reaches target_tick
void wait_until_tick(int target_tick) {
    int rc = pthread_mutex_lock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_lock");
        exit(EXIT_FAILURE);
    }
    while (global_tick < target_tick) {
        // Atomically releases lock + sleeps; re-acquires on wake
        rc = pthread_cond_wait(&tick_changed, &tick_lock);
        if (rc != 0) {
            errno = rc;
            perror("pthread_cond_wait");
            pthread_mutex_unlock(&tick_lock);
            exit(EXIT_FAILURE);
        }
    }
    rc = pthread_mutex_unlock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
}

// Tell the timer thread to stop
void stop_timer(void) {
    int rc = pthread_mutex_lock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_lock");
        exit(EXIT_FAILURE);
    }

    atomic_store(&simulation_running, 0);

    rc = pthread_cond_broadcast(&tick_changed);
    if (rc != 0) {
        errno = rc;
        perror("pthread_cond_broadcast");
        pthread_mutex_unlock(&tick_lock);
        exit(EXIT_FAILURE);
    }

    rc = pthread_mutex_unlock(&tick_lock);
    if (rc != 0) {
        errno = rc;
        perror("pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
}

// Wait for the timer thread to exit
void join_timer(void) {
    int rc = pthread_join(timer_tid, NULL);
    if (rc != 0) {
        errno = rc;
        perror("pthread_join");
        exit(EXIT_FAILURE);
    }
}

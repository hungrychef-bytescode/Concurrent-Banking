#include <stdio.h>
#include <unistd.h>
#include "timer.h"

// Global definitions (declared extern in timer.h)
volatile int    global_tick       = 0;
int             simulation_running = 1;
pthread_mutex_t tick_lock         = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  tick_changed      = PTHREAD_COND_INITIALIZER;

static pthread_t timer_tid;
static int       tick_interval_ms_global;

// The timer thread: increments global_tick every tick_interval_ms
static void* timer_thread(void *arg) {
    (void)arg;

    while (simulation_running) {
        usleep(tick_interval_ms_global * 1000);  // sleep one tick

        pthread_mutex_lock(&tick_lock);
        global_tick++;
        pthread_cond_broadcast(&tick_changed);   // wake all waiting threads
        pthread_mutex_unlock(&tick_lock);
    }

    return NULL;
}

// Start the timer thread
void init_timer(int tick_interval_ms) {
    tick_interval_ms_global = tick_interval_ms;
    pthread_create(&timer_tid, NULL, timer_thread, NULL);
}

// Block until the global clock reaches target_tick
void wait_until_tick(int target_tick) {
    pthread_mutex_lock(&tick_lock);
    while (global_tick < target_tick) {
        // Atomically releases lock + sleeps; re-acquires on wake
        pthread_cond_wait(&tick_changed, &tick_lock);
    }
    pthread_mutex_unlock(&tick_lock);
}

// Tell the timer thread to stop
void stop_timer(void) {
    simulation_running = 0;
}

// Wait for the timer thread to exit
void join_timer(void) {
    pthread_join(timer_tid, NULL);
}
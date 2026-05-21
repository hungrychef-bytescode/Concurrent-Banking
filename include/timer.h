#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdatomic.h>

// Global simulation clock — shared by all threads
extern atomic_int global_tick;
extern atomic_int simulation_running;
extern pthread_mutex_t tick_lock;
extern pthread_cond_t  tick_changed;

// Start the timer thread (call once from main)
void init_timer(int tick_interval_ms);

// Block the calling thread until global_tick >= target_tick
void wait_until_tick(int target_tick);

// Return the current global tick in a thread-safe way
int get_current_tick(void);

// Signal the timer to stop
void stop_timer(void);

// Join the timer thread (call after all transactions are done)
void join_timer(void);

#endif
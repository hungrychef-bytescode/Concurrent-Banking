#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>

typedef struct {
    int current_tick;
    int tick_ms;
    int running;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t thread;
} Timer;

void timer_init(Timer *t, int tick_ms);
void timer_start(Timer *t);
void timer_stop(Timer *t);

void timer_wait_for_tick(Timer *t, int target_tick);

#endif
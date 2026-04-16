#include "timer.h"
#include <unistd.h>

void *timer_thread_func(void *arg) {
    Timer *t = (Timer *)arg;

    while (t->running) {
        usleep(t->tick_ms * 1000);

        pthread_mutex_lock(&t->mutex);
        t->current_tick++;
        pthread_cond_broadcast(&t->cond);
        pthread_mutex_unlock(&t->mutex);
    }

    return NULL;
}

void timer_init(Timer *t, int tick_ms) {
    t->current_tick = 0;
    t->tick_ms = tick_ms;
    t->running = 1;

    pthread_mutex_init(&t->mutex, NULL);
    pthread_cond_init(&t->cond, NULL);
}

void timer_start(Timer *t) {
    pthread_create(&t->thread, NULL, timer_thread_func, t);
}

void timer_stop(Timer *t) {
    t->running = 0;
    pthread_join(t->thread, NULL);

    pthread_mutex_destroy(&t->mutex);
    pthread_cond_destroy(&t->cond);
}

void timer_wait_for_tick(Timer *t, int target_tick) {
    pthread_mutex_lock(&t->mutex);

    while (t->current_tick < target_tick) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }

    pthread_mutex_unlock(&t->mutex);
}
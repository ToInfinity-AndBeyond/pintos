#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include <list.h>
#include "threads/synch.h"

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

/* Struct for a thread that is sleeping. */
struct sleeping_thread
{
    int64_t wakeup_tick;            /* Number of ticks sleeping thread needs to be awaken. */
    struct list_elem element;       /* List element for all sleeping thread list. */
    struct semaphore sleeping_sema; /* Semaphore used for managing block state of the thread. */
};

void timer_init(void);
void timer_calibrate(void);

int64_t timer_ticks(void);
int64_t timer_elapsed(int64_t);

/* Sleep and yield the CPU to other threads. */
void timer_sleep(int64_t ticks);
/* Helper function of timer_sleep. Describes the process of timer_sleep. */
void timer_sleep_process(int64_t wakeup_tick);
void timer_msleep(int64_t milliseconds);
void timer_usleep(int64_t microseconds);
void timer_nsleep(int64_t nanoseconds);

/* Compare wakeup_tick of two list_elem elements. */
bool compare_wakeup_tick(const struct list_elem *a,
                         const struct list_elem *b,
                         void *aux);

/* Given ticks parameter, wakes up the sleeping threads that meet the condition. */
void thread_awake(int64_t ticks);

/* Busy waits. */
void timer_mdelay(int64_t milliseconds);
void timer_udelay(int64_t microseconds);
void timer_ndelay(int64_t nanoseconds);

void timer_print_stats(void);

#endif /* devices/timer.h */

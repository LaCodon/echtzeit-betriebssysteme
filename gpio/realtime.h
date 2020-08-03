#ifndef GPIO_REALTIME_H
#define GPIO_REALTIME_H

#define _GNU_SOURCE

#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define THREAD_STACK_SIZE (256*1024)

#define ERROR_PTH_ATTRS_FAILED 1
#define ERROR_PTH_SETSTACKSIZE_FAILED 2
#define ERROR_PTH_SETSCHEDPOLICY_FAILED 3
#define ERROR_PTH_SETINHERITSCHED_FAILED 4
#define ERROR_PTH_SETSCHEDPARAM_FAILED 5
#define ERROR_PTH_THREADCREATE_FAILED 6

#define nullptr ((void*)0)

#ifndef VERBOSE
#define VERBOSE 1
#endif

#ifdef VERBOSE
#define PRINT_START(i) printf("Started #%d\n", i);
#define PRINT_END(i) printf("Ended #%d\n", i);
#else
#define PRINT_START(i) nullptr;
#define PRINT_END(i) nullptr;
#endif

typedef void (*callbk_t)();

typedef struct {
    bool cond;
    pthread_cond_t pthreadCond;
    pthread_mutex_t pthreadMutex;
} cond_wait_t;

typedef struct {
    callbk_t func;
    void *arg;
} thread_t;

// Start a given function as realtime thread on cpuset with priority
extern int start_realtime_thread(pthread_t *pthread, thread_t *func, cpu_set_t *cpuset, int priority);

#endif //GPIO_REALTIME_H

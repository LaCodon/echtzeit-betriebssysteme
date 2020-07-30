#ifndef GPIO_REALTIME_H
#define GPIO_REALTIME_H

#define _GNU_SOURCE

#include <sched.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_STACK_SIZE (256*1024)

#define ERROR_PTH_ATTRS_FAILED 1
#define ERROR_PTH_SETSTACKSIZE_FAILED 2
#define ERROR_PTH_SETSCHEDPOLICY_FAILED 3
#define ERROR_PTH_SETINHERITSCHED_FAILED 4
#define ERROR_PTH_SETSCHEDPARAM_FAILED 5
#define ERROR_PTH_THREADCREATE_FAILED 6

// Start a given function as realtime thread on cpuset with priority
extern int start_realtime_thread(pthread_t *pthread, void *func, cpu_set_t *cpuset, int priority);

#endif //GPIO_REALTIME_H

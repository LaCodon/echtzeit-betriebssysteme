
#include "realtime.h"


void *thread_start_helper(void *arg) {
    // sleep to give system time to migrate thread to correct cpu
    usleep(300);

    // call user defined thread function
    thread_t *thread = arg;
    if (thread->arg == nullptr) {
        thread->func();
    } else {
        thread->func(thread->arg);
    }
}

int start_realtime_thread(pthread_t *pthread, thread_t *func, cpu_set_t *cpuset, int priority) {
    struct sched_param param;
    pthread_attr_t attr;

    // Initialize pthread attributes (default values)
    if (pthread_attr_init(&attr)) {
        return ERROR_PTH_ATTRS_FAILED;
    }

    // Set a specific stack size
    if (pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE)) {
        return ERROR_PTH_SETSTACKSIZE_FAILED;
    }

    // Set scheduler policy and priority of pthread
    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
        return ERROR_PTH_SETSCHEDPOLICY_FAILED;
    }

    // Use scheduling parameters of attr
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
        return ERROR_PTH_SETINHERITSCHED_FAILED;
    }

    // Set thread priority
    param.sched_priority = priority;
    if (pthread_attr_setschedparam(&attr, &param)) {
        return ERROR_PTH_SETSCHEDPARAM_FAILED;
    }

    // Start thread
    if (pthread_create(pthread, &attr, thread_start_helper, func)) {
        return ERROR_PTH_THREADCREATE_FAILED;
    }

    // Set CPU affinity
    pthread_setaffinity_np(*pthread, sizeof(cpu_set_t), cpuset);

    return 0;
}

struct timespec diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

#define _GNU_SOURCE

#include <pthread.h>
#include "gpio.h"

#define STACK_SIZE (256*1024)
#define GPIO_COUNT 50

typedef void (*callbk_t)();

typedef struct {
    unsigned int flankCounter;
    unsigned int gpio;
    pthread_t pth;
    thread_t thread;
    callbk_t func;
    int timeout;
    int fd;
    unsigned int edge;
    cond_wait_t *condWait;
} gpioISR_t;

gpioISR_t gpioISR[GPIO_COUNT];

// Init peripheral data struct
struct bcm2837_peripheral gpio = {GPIO_BASE};

// Access physical memory via /dev/mem (kernel call)
int map_peripherals() {
    if ((gpio.mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        printf("Fehler beim Öffnen von /dev/mem. Überprüfe Berechtigungen.\n");
        return -1;
    }

    // mmap creates a new mapping in the virtual address space
    gpio.map = mmap(
            NULL,
            BLOCK_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            gpio.mem_fd,
            gpio.addr_p
    );

    // file descriptor can be closed immediately according to man page
    close(gpio.mem_fd);

    if (gpio.map == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    gpio.addr = (volatile unsigned int *) gpio.map;

    return 0;
}

// Remove physical memory mapping
void unmap_peripherals() {
    munmap(gpio.map, BLOCK_SIZE);
}

// Simulates interrupts via polling
_Noreturn static void pthISRThread(void *x) {
    gpioISR_t *isr = x;
    int retval;
    struct pollfd pfd;
    int fd;
    char buf[64];
    int level;

    // file to poll
    sprintf(buf, "/sys/class/gpio/gpio%d/value", isr->gpio);

    isr->fd = -1;

    if ((fd = open(buf, O_RDONLY)) < 0) {
        printf("Failed to get GPIO value");
    }

    // store fd because it has to be closed after stopping this thread
    isr->fd = fd;

    pfd.fd = fd;

    pfd.events = POLLPRI;

    // consume any prior interrupt
    lseek(fd, 0, SEEK_SET);
    if (read(fd, buf, sizeof buf) == -1) { /* ignore errors */ }

    while (1) {
        if (isr->condWait != nullptr) {
            pthread_mutex_lock(&isr->condWait->pthreadMutex);
            while (!isr->condWait->cond)
                pthread_cond_wait(&isr->condWait->pthreadCond, &isr->condWait->pthreadMutex);
        }
        while (isr->condWait->cond) {

            // wait for file change event ("interrupt")
            retval = poll(&pfd, 1, isr->timeout);

            if (retval >= 0) {
                // consume interrupt
                lseek(fd, 0, SEEK_SET);
                if (read(fd, buf, sizeof buf) == -1) { /* ignore errors */ }

                if (retval) {
                    if (isr->edge == EDGE_RISING) level = GPIO_ON; else level = GPIO_OFF;
                } else level = GPIO_TIMEOUT;

                // call user defined handler
                (isr->func)(isr->gpio, level);
            } else {
                printf("poll return error\n");
            }

        }
        if (isr->condWait != nullptr) {
            pthread_mutex_unlock(&isr->condWait->pthreadMutex);
        }
    }
}

// Setup GPIO and start listening for interrupts
int init_isr_func(unsigned int pin, unsigned int edge, void *f,
                  cond_wait_t *condWait, cpu_set_t *cpuset, int priority) {
    int fd;
    int err;
    char buf[64];
    char *edge_str[] = {"rising\n", "falling\n", "both\n"};

    // do nothing if thread is already running
    if (gpioISR[pin].pth != 0) return 1;

    // enblae gpio
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) return ERROR_EXPORT_FAIL;

    /* ignore write fail if already exported */
    sprintf(buf, "%d\n", pin);
    write(fd, buf, strlen(buf));
    close(fd);

    // set as input
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(buf, O_WRONLY);
    if (fd < 0) return ERROR_DIRECTION_FAIL;

    err = write(fd, "in\n", 3);
    close(fd);
    if (err != 3) return 1;

    // set edge detection
    sprintf(buf, "/sys/class/gpio/gpio%d/edge", pin);
    fd = open(buf, O_WRONLY);
    if (fd < 0) return ERROR_EDGE_FAIL;
    write(fd, edge_str[edge], strlen(edge_str[edge]));
    close(fd);

    // start listening for interrupts on pin
    thread_t thread = {pthISRThread, &gpioISR[pin]};
    gpioISR[pin].gpio = pin;
    gpioISR[pin].thread = thread;
    gpioISR[pin].func = f;
    gpioISR[pin].timeout = 1000;
    gpioISR[pin].edge = edge;
    gpioISR[pin].condWait = condWait;

    start_realtime_thread(&gpioISR[pin].pth, &gpioISR[pin].thread, cpuset, priority);

    if (gpioISR[pin].pth == 0) {
        return ERROR_THREAD_ALLOC_FAIL;
    }

    return 0;
}

// Stop listening for interrupts and clean resources
int del_isr_func(unsigned int pin) {
    if (gpioISR[pin].pth == 0) return ERROR_ISR_NOT_INITED;

    pthread_cancel(gpioISR[pin].pth);
    pthread_join(gpioISR[pin].pth, NULL);

    gpioISR[pin].gpio = 0;
    gpioISR[pin].thread = (thread_t) {0, nullptr};
    gpioISR[pin].timeout = 0;
    gpioISR[pin].edge = 0;
    close(gpioISR[pin].fd);
    gpioISR[pin].pth = 0;

    return 0;
}

// Return monotonic clock time in us
uint64_t get_clock_time() {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    else
        return 0;
}

// Helper ISR for read_input_freq()
void freq_counter(int pin, int level) {
    if (level != GPIO_TIMEOUT) gpioISR[pin].flankCounter++;
}

double read_input_freq(int pin, useconds_t sampleinterval, cond_wait_t *cond) {
    uint64_t prev_time_value, time_value;
    double time_diff;
    double freq;
    int err;

    gpioISR[pin].flankCounter = 0;
    prev_time_value = get_clock_time();

    cond->cond = true;
    pthread_cond_signal(&cond->pthreadCond);
    //err = init_isr_func(pin, EDGE_RISING, counter, nullptr, cpuset, priority);
    // count interrupts for sampleinterval us
    usleep(sampleinterval);
    //del_isr_func(pin);

    cond->cond = false;
    time_value = get_clock_time(); // in us
    time_diff = (time_value - prev_time_value); // in us

    freq = ((gpioISR[pin].flankCounter / time_diff) * 1000000);


    return freq;
}
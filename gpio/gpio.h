#ifndef GPIO_GPIO_H
#define GPIO_GPIO_H

#include <stdio.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <pthread.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// start of physical address space for peripherals
#define BCM2837_PERI_BASE   0x3f000000
// start of GPIO memory space
#define GPIO_BASE           (BCM2837_PERI_BASE + 0x200000)

#define BLOCK_SIZE          (4*1024)

#define EDGE_RISING         0
#define EDGE_FALLING        1
#define EDGE_BOTH           2

#define GPIO_OFF            0
#define GPIO_ON             1
#define GPIO_TIMEOUT        2

#define ERROR_EXPORT_FAIL           10
#define ERROR_DIRECTION_FAIL        11
#define ERROR_EDGE_FAIL             12
#define ERROR_THREAD_ALLOC_FAIL     13
#define ERROR_ISR_NOT_INITED        14

#define DEFAULT_SAMPLE_TIME     50000

#define nullptr ((void*)0)

typedef struct {
    bool cond;
    pthread_cond_t pthreadCond;
    pthread_mutex_t pthreadMutex;
} cond_wait_t;

// Periphery access struct
struct bcm2837_peripheral {
    unsigned long addr_p; // start address of GPIO memory
    int mem_fd; // file descriptor to referer memory file /dev/mem
    void *map; // pointer to actual map for later unmap
    volatile unsigned int *addr; // start address of mapped memory
};

extern struct bcm2837_peripheral gpio;

// Macros for GPIO access
#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g, a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET  *(gpio.addr + 7)  // set high bits and ignore low ones
#define GPIO_CLR  *(gpio.addr + 10) // clears high bits and ignore low ones

#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))
#define GPIO_PULL  *(gpio.addr + 37)  // pull up and pull down activation
#define GPIO_PULLCLK(g) *(gpio.addr + 38) &= (1<<(g)) // clock pull up or pull down

// Map peripherals via mmap
extern int map_peripherals();

// Unmap peripherals memory
extern void unmap_peripherals();

// Listen for new interrupts on pin
extern int init_isr_func(unsigned int pin, unsigned int edge, void *f,
        cond_wait_t *condWait, cpu_set_t *cpuset, int priority);

// Stop listening for interrupts
extern int del_isr_func(unsigned int pin);

// Measure input frequency on pin in Hz for sampleintervall us
extern double read_input_freq(unsigned int pin, useconds_t sampleinterval, cpu_set_t *cpuset, int priority);

// Get current monotonic clock time in us
extern uint64_t get_clock_time();

#endif //GPIO_GPIO_H

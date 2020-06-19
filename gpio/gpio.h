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

//phys. Adressraum/Startadresse für Periperiekomponenten, wie USB, GPIO
#define BCM2837_PERI_BASE   0x3f000000
//phys. Adressraum/Startadresse für GPIO
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

//IO Zugriff Struct
struct bcm2837_peripheral {
    unsigned long addr_p; // start address of GPIO memory
    int mem_fd; //file descriptor to referer memory file /dev/mem
    void *map;
    volatile unsigned int *addr; // start address of mapped memory
};
//volatile sorgt dafür, dass Wert im aus Hauptspeicher geholt wird und
//nicht aus irgend ein Register in CPU;

extern struct bcm2837_peripheral gpio;

// Makros für GPIO-Zugriff; immer INP_GPIO(x) vor OUT_GPIO(x) benutzen
#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g, a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET  *(gpio.addr + 7)  // setzt Bits die 1 sind und ignoriert Bits die 0 sind
#define GPIO_CLR  *(gpio.addr + 10) // löscht Bits die 1 sind und ignoriert Bits die 0 sind

#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))
#define GPIO_PULL  *(gpio.addr + 37)  //Pull up oder Pull Down Aktivierungen
#define GPIO_PULLCLK(g) *(gpio.addr + 38) &= (1<<(g)) //Clock pull up oder pull down

// Map peripherals via mmap
extern int map_peripherals(struct bcm2837_peripheral *p);

// Unmap peripherals memory
extern void unmap_peripherals(struct bcm2837_peripheral *p);

// Listen for new interrupts on pin
extern int init_isr_func(unsigned int pin, unsigned int edge, void *f);

// Stop listening for interrupts
extern int del_isr_func(unsigned int pin);

// Measure input frequency on pin in Hz for sampleintervall us
extern double read_input_freq(unsigned int pin, useconds_t sampleinterval);

// get current monotonic clock time in us
extern uint64_t get_clock_time();

#endif //GPIO_GPIO_H

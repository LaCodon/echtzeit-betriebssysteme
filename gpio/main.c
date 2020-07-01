#include <sys/mman.h>
#include <time.h>
#include <stdint.h>

#include "gpio.h"

#define PIN17 17
#define PIN18 18

// The datasheet says 5880 square waves per litre but I measured something different
#define RISING_EDGE_PER_LITRE 4880

volatile int count = 0;

void isr(int pin, int level) {
    if (level == GPIO_ON) {
        count++;

        if (count >= RISING_EDGE_PER_LITRE) {
            GPIO_SET |= 1 << PIN17;
        }
    }
}

int main() {

    if (map_peripherals() == -1) {
        printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
        return -1;
    }

    INP_GPIO(PIN18);
    //OUT_GPIO(PIN18);

    INP_GPIO(PIN17);
    OUT_GPIO(PIN17);
    GPIO_CLR |= 1 << PIN17;


    init_isr_func(PIN18, EDGE_RISING, isr);

    while (1) {
//        usleep(950000);
//        GPIO_SET |= 1 << PIN18;
//
//        GPIO_CLR |= 1 << PIN18;

//        double freq = read_input_freq(PIN17, DEFAULT_SAMPLE_TIME) * 16;
//        printf("%.2f Hz\n", freq);

        double litre = (double) count / RISING_EDGE_PER_LITRE;
        printf("count: %d  -> %.3f Liter\n", count, litre);

        if (litre >= 1) {
            break;
        }
//
        sleep(1);

//        if (GPIO_READ(PIN18)) {
//            printf("HIGH\n");
//        } else {
//            printf("LOW\n");
//        }
    }


    unmap_peripherals();

    return 0;
}

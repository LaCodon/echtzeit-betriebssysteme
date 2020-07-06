#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>

#include "gpio.h"
#include "ui.h"

// humidity sensor
#define HUMIDITY_SENSOR 17
// flow counter
#define FLOW_SENSOR 18
// pump
#define PUMP 27

// The datasheet says 5880 square waves per litre but I measured something different
#define RISING_EDGE_PER_LITRE 4880
// Time to not water again after watering in seconds
#define NO_WATER_AFTER_WATERING 120

volatile int water_count = 0;
struct config_data config;

void water_count_isr(int pin, int level) {
    if (level == GPIO_ON) {
        water_count++;

        if (((double) water_count / RISING_EDGE_PER_LITRE) * (double) 1000 >= config.milliliters) {
            // stop pump
            GPIO_SET |= 1 << PUMP;
        }
    }
}

_Noreturn void *reload_config() {
    while (1) {
        // reload config from calibration.csv every 5 seconds
        sleep(5);
        load_config(&config);
        printf("%ld - %ld - %ld\n", config.arid, config.humid, config.milliliters);
    }
}

int start() {

    if (map_peripherals() == -1) {
        printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
        return -1;
    }

    INP_GPIO(HUMIDITY_SENSOR);
    INP_GPIO(FLOW_SENSOR);

    INP_GPIO(PUMP);
    OUT_GPIO(PUMP);

    // stop pump
    GPIO_SET |= 1 << PUMP;

    // make sure calibration.csv and test-hydro.csv exist in this directory
    // don't forget the trailing slash in the path!
    set_ui_dir("/home/pi/gpio_data/");

    // initial config load to make sure variable is set
    load_config(&config);

    pthread_t configReloadThread;
    pthread_create(&configReloadThread, NULL, reload_config, 0);

    double freq = 0;
    int no_water_count = 0;

    while (1) {
        if (no_water_count > 0)
            no_water_count--;

        // check humidity
        if ((long int) freq > config.arid && no_water_count == 0) {
            // we have to water the plant

            // start measuring water flow
            water_count = 0;
            init_isr_func(FLOW_SENSOR, EDGE_RISING, water_count_isr);

            // start pump
            GPIO_CLR |= 1 << PUMP;

            while (1) {
                freq = read_input_freq(HUMIDITY_SENSOR, DEFAULT_SAMPLE_TIME) * 16;
                printf("%.2f Hz\n", freq);
                send_freq_to_ui(freq);

                double millilitre = ((double) water_count / RISING_EDGE_PER_LITRE) * (double) 1000;
                printf("%.0f Millilitre\n", water_count, millilitre);

                if (millilitre >= config.milliliters) {
                    break;
                }

                // we have to substract DEFAULT_SAMPLE_TIME because this
                // is the time the read_input_freq function needs for
                // the frequency measurement
                usleep(1 * 1000 * 1000 - DEFAULT_SAMPLE_TIME);
            }

            // stop water flow measurement
            del_isr_func(FLOW_SENSOR);

            // don't water for longer period to let water soak into earth
            no_water_count = NO_WATER_AFTER_WATERING;
        } else {
            // just send humidity to dashboard
            freq = read_input_freq(HUMIDITY_SENSOR, DEFAULT_SAMPLE_TIME) * 16;
            printf("%.2f Hz\n", freq);
            send_freq_to_ui(freq);

            usleep(1 * 1000 * 1000 - DEFAULT_SAMPLE_TIME);
        }
    }


    pthread_cancel(configReloadThread);
    unmap_peripherals();

    return 0;
}

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }

    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 80;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        goto out;
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }

    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread, &attr, start, NULL);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }

    /* Join the thread and wait until it is done */
    ret = pthread_join(thread, NULL);
    if (ret)
        printf("join pthread failed: %m\n");

    out:
    return ret;
}

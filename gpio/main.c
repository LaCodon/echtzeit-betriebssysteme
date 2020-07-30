#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#define _GNU_SOURCE

#include <sys/mman.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <sched.h>

#include "gpio.h"
#include "ui.h"

// humidity sensor
#define HUMIDITY_SENSOR 17
// flow counter
#define FLOW_SENSOR 18
// pump
#define PUMP 27

#define WATER_COUNT_PRIO 80
#define RELOAD_CONFIG_PRIO 60
#define CHECK_HUMIDITY_PRIO 70
#define READ_HUMIDITY_FREQUENCY_PRIO 72

// The datasheet says 5880 square waves per litre but I measured something different
#define RISING_EDGE_PER_LITRE 4880
// Time to not water again after watering in seconds
#define NO_WATER_AFTER_WATERING 120

#define VERBOSE

volatile int water_count = 0;
bool isWatering = false;
struct config_data config;

cpu_set_t cpuset;
cond_wait_t checkHumidityCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
cond_wait_t reloadConfigCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
cond_wait_t waterCountCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

void water_count_isr(int pin, int level) {
    if (level == GPIO_ON) {
        water_count++;

        if (((double) water_count / RISING_EDGE_PER_LITRE) * (double) 1000 >= config.milliliters) {
            // stop pump
            GPIO_SET |= 1 << PUMP;
            waterCountCond.cond = false;
            isWatering = false;
        }
    }
}

_Noreturn void *reload_config() {
    while (1) {
        pthread_mutex_lock(&reloadConfigCond.pthreadMutex);
        while (!reloadConfigCond.cond)
            pthread_cond_wait(&reloadConfigCond.pthreadCond, &reloadConfigCond.pthreadMutex);
        reloadConfigCond.cond = false;
        // reload config from calibration.csv every 5 seconds
        sleep(5);
        load_config(&config);
#ifdef VERBOSE
        printf("%ld - %ld - %ld\n", config.arid, config.humid, config.milliliters);
#endif
        pthread_mutex_unlock(&reloadConfigCond.pthreadMutex);
    }
}

/*_Noreturn void * start(void *pVoid) {

    if (map_peripherals() == -1) {
        printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
        return (void *) -1;
    }

    INP_GPIO(HUMIDITY_SENSOR);
    INP_GPIO(FLOW_SENSOR);

    INP_GPIO(PUMP);
    OUT_GPIO(PUMP);

    // stop pump
    GPIO_SET |= 1 << PUMP;

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
            init_isr_func(FLOW_SENSOR, EDGE_RISING, water_count_isr, nullptr);

            // start pump
            GPIO_CLR |= 1 << PUMP;

            while (1) {
                freq = read_input_freq(HUMIDITY_SENSOR, DEFAULT_SAMPLE_TIME) * 16;
                printf("%.2f Hz\n", freq);
                send_freq_to_ui(freq);

                double millilitre = ((double) water_count / RISING_EDGE_PER_LITRE) * (double) 1000;
                printf("%.0f Millilitre\n", millilitre);

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

    unmap_peripherals();

    return 0;
}*/

// call
_Noreturn void* check_humidity()
{
    double freq;
    while(1){
        pthread_mutex_lock(&checkHumidityCond.pthreadMutex);
        while (!checkHumidityCond.cond)
            pthread_cond_wait(&checkHumidityCond.pthreadCond, &checkHumidityCond.pthreadMutex);
        checkHumidityCond.cond = false;
        //get frequency from sensor
        freq = read_input_freq(HUMIDITY_SENSOR, DEFAULT_SAMPLE_TIME, &cpuset, READ_HUMIDITY_FREQUENCY_PRIO) * 16;
#ifdef VERBOSE
        printf("%.2f Hz\n", freq);
#endif
        send_freq_to_ui(freq);

        if(freq > config.arid && !waterCountCond.cond)
        {
            waterCountCond.cond = true;
        }
        pthread_mutex_unlock(&checkHumidityCond.pthreadMutex);
    }
}

void startAllThreads() {
    checkHumidityCond.cond = true;
    reloadConfigCond.cond = true;
    pthread_cond_signal(&checkHumidityCond.pthreadCond);
    pthread_cond_signal(&reloadConfigCond.pthreadCond);
    if(waterCountCond.cond && !isWatering) {
        isWatering = true;
        printf("Starting pump thread\n");
        water_count = 0;
        GPIO_CLR |= 1 << PUMP;
        pthread_cond_signal(&waterCountCond.pthreadCond);
    }
}

int main(int argc, char* argv[])
{
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t checkHumidityThread;
    pthread_t configReloadThread;

    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);


    // make sure calibration.csv and test-hydro.csv exist in this directory
    // don't forget the trailing slash in the path!
    set_ui_dir("/home/pi/gpio_data/");

    // initial config load to make sure variable is set
    load_config(&config);

    /* Lock memory */
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }

    /* Initialize pthread attributes (default values) */
    if (pthread_attr_init(&attr)){
        printf("init pthread attributes failed\n");
        goto out;
    }

    /* Set a specific stack size  */
    if (pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN*4)){
        printf("pthread setstacksize failed\n");
        goto out;
    }

    /* Set scheduler policy and priority of pthread */
    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    /* Use scheduling parameters of attr */
    if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
        printf("pthreaf setinheritsched failed\n");
        goto out;
    }

    param.sched_priority = RELOAD_CONFIG_PRIO;
    if (pthread_attr_setschedparam(&attr, &param)) {
        printf("pthread setschedparam failed\n");
    }
    pthread_create(&configReloadThread, &attr, reload_config, NULL);
    pthread_setaffinity_np(configReloadThread, sizeof(cpu_set_t), &cpuset);

    param.sched_priority = CHECK_HUMIDITY_PRIO;
    if (pthread_attr_setschedparam(&attr, &param)) {
        printf("pthread setschedparam failed\n");
    }
    pthread_create(&checkHumidityThread, &attr, check_humidity, NULL);
    pthread_setaffinity_np(configReloadThread, sizeof(cpu_set_t), &cpuset);

    //initialize gpios
    if (map_peripherals() == -1) {
        printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
        return (void *) -1;
    }

    INP_GPIO(HUMIDITY_SENSOR);
    INP_GPIO(FLOW_SENSOR);

    INP_GPIO(PUMP);
    OUT_GPIO(PUMP);

    // stop pump
    GPIO_SET |= 1 << PUMP;

    init_isr_func(FLOW_SENSOR, EDGE_RISING, water_count_isr, &waterCountCond, &cpuset, WATER_COUNT_PRIO);

    //now schedule
    while (1)
    {
        startAllThreads();
        sleep(10);
    }

    out:
    return 0;
}

#pragma clang diagnostic pop
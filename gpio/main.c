#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#define _GNU_SOURCE

#include <sys/mman.h>
#include <stdbool.h>
#include <sched.h>

#include "gpio.h"
#include "ui.h"
#include "realtime.h"

// humidity sensor
#define HUMIDITY_SENSOR 17
// flow counter
#define FLOW_SENSOR 18
// pump
#define PUMP 27

#define PERIODE_DURATION 120
#define MAIN_PRIO 90
#define WATER_COUNT_PRIO 80
#define RELOAD_CONFIG_PRIO 60
#define CHECK_HUMIDITY_PRIO 75
#define READ_HUMIDITY_FREQUENCY_PRIO 70

// The datasheet says 5880 square waves per litre but I measured something different
#define RISING_EDGE_PER_LITRE 4880

volatile int water_count = 0;
bool isWatering = false;
struct config_data config;

cpu_set_t cpuset;
cond_wait_t checkHumidityCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
cond_wait_t reloadConfigCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
cond_wait_t waterCountCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
cond_wait_t readFrequencyCond = {false, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

void water_count_isr(int pin, int level) {
    if (level == GPIO_ON) {
        water_count++;
        printf(".");

        if (((double) water_count / RISING_EDGE_PER_LITRE) * (double) 1000 >= config.milliliters) {
            // stop pump
            GPIO_SET |= 1 << PUMP;
            waterCountCond.cond = false;
            isWatering = false;
        }
    }
}

_Noreturn void reload_config() {
#ifdef TIMER
    struct timespec startTime, endTime, diffTime;
    clockid_t threadClockId;
    pthread_getcpuclockid(pthread_self(), &threadClockId);
#endif
    while (1) {
        pthread_mutex_lock(&reloadConfigCond.pthreadMutex);
        while (!reloadConfigCond.cond)
            pthread_cond_wait(&reloadConfigCond.pthreadCond, &reloadConfigCond.pthreadMutex);
        PRINT_START(4)
#ifdef TIMER
        clock_gettime(threadClockId, &startTime);
#endif
        reloadConfigCond.cond = false;
        load_config(&config);
#ifdef VERBOSE
        printf("%ld - %ld - %ld\n", config.arid, config.humid, config.milliliters);
#endif
#ifdef TIMER
        clock_gettime(threadClockId, &endTime);
        diffTime = diff(startTime, endTime);
        printf("Config Thread time: %ld:%ld\n", diffTime.tv_sec, diffTime.tv_nsec);
#endif
        PRINT_END(4)
        pthread_mutex_unlock(&reloadConfigCond.pthreadMutex);
    }
}

_Noreturn void check_humidity() {
#ifdef TIMER
    struct timespec startTime, endTime, diffTime;
    clockid_t threadClockId;
    pthread_getcpuclockid(pthread_self(), &threadClockId);
#endif
    double freq;
    while (1) {
        pthread_mutex_lock(&checkHumidityCond.pthreadMutex);
        while (!checkHumidityCond.cond)
            pthread_cond_wait(&checkHumidityCond.pthreadCond, &checkHumidityCond.pthreadMutex);
        PRINT_START(8)
#ifdef TIMER
        clock_gettime(threadClockId, &startTime);
#endif
        checkHumidityCond.cond = false;
        //get frequency from sensor
        freq = read_input_freq(HUMIDITY_SENSOR, DEFAULT_SAMPLE_TIME, &readFrequencyCond) * 16;
#ifdef VERBOSE
        printf("%.2f Hz\n", freq);
#endif
        send_freq_to_ui(freq);

        if (freq > config.arid && !waterCountCond.cond) {
            waterCountCond.cond = true;
        }
#ifdef TIMER
        clock_gettime(threadClockId, &endTime);
        diffTime = diff(startTime, endTime);
        printf("Humidity Thread time: %ld:%ld\n", diffTime.tv_sec, diffTime.tv_nsec);
#endif
        PRINT_END(8)
        pthread_mutex_unlock(&checkHumidityCond.pthreadMutex);
    }
}

void startAllThreads() {
    checkHumidityCond.cond = true;
    reloadConfigCond.cond = true;
    pthread_cond_signal(&checkHumidityCond.pthreadCond);
    pthread_cond_signal(&reloadConfigCond.pthreadCond);
    if (waterCountCond.cond && !isWatering) {
        isWatering = true;
        printf("Starting pump thread\n");
        water_count = 0;
        GPIO_CLR |= 1 << PUMP;
        pthread_cond_signal(&waterCountCond.pthreadCond);
    }
}

_Noreturn void main_thread() {
#ifdef TIMER
    struct timespec startTime, endTime, diffTime;
    clockid_t threadClockId;
    pthread_getcpuclockid(pthread_self(), &threadClockId);
#endif
    while (1) {
#ifdef TIMER
        clock_gettime(threadClockId, &startTime);
#endif
		printf("============================\n");
        startAllThreads();
        sleep(PERIODE_DURATION);
        // stop pumping because of deadline
        if(isWatering)
        {
            GPIO_SET |= 1 << PUMP;
            waterCountCond.cond = false;
            isWatering = false;
        }
#ifdef TIMER
        clock_gettime(threadClockId, &endTime);
        diffTime = diff(startTime, endTime);
        printf("Main Thread time: %ld:%ld\n", diffTime.tv_sec, diffTime.tv_nsec);
#endif
    }
}

int main(int argc, char *argv[]) {
    pthread_t checkHumidityPThread;
    pthread_t configReloadPThread;
    pthread_t mainPThread;

    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);


    // make sure calibration.csv and test-hydro.csv exist in this directory
    // don't forget the trailing slash in the path!
    set_ui_dir("/home/pi/gpio_data/");

    // initial config load to make sure variable is set
    load_config(&config);

    //initialize gpios
    if (map_peripherals() == -1) {
        printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
        return 1;
    }

    // lock memory to not get swapped
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        return 1;
    }

    thread_t configReloadThread = {reload_config, nullptr};
    if (start_realtime_thread(&configReloadPThread, &configReloadThread, &cpuset, RELOAD_CONFIG_PRIO)) {
        printf("Failed to start RT configReloadThread");
        return 1;
    }

    thread_t checkHumidityThread = {check_humidity, nullptr};
    if (start_realtime_thread(&checkHumidityPThread, &checkHumidityThread, &cpuset, CHECK_HUMIDITY_PRIO)) {
        printf("Failed to start RT checkHumidityThread");
        return 1;
    }

    INP_GPIO(HUMIDITY_SENSOR);
    INP_GPIO(FLOW_SENSOR);

    INP_GPIO(PUMP);
    OUT_GPIO(PUMP);

    // stop pump
    GPIO_SET |= 1 << PUMP;

    init_isr_func(FLOW_SENSOR, EDGE_RISING, water_count_isr, &waterCountCond, &cpuset, WATER_COUNT_PRIO);
    init_isr_func(HUMIDITY_SENSOR, EDGE_RISING, freq_counter, &readFrequencyCond, &cpuset,
                  READ_HUMIDITY_FREQUENCY_PRIO);


    thread_t mainThread = {main_thread, nullptr};
    if (start_realtime_thread(&mainPThread, &mainThread, &cpuset, MAIN_PRIO)) {
        printf("Failed to start RT checkHumidityThread");
        return 1;
    }
    
    pthread_join(mainPThread, NULL);

    return 0;
}

#pragma clang diagnostic pop
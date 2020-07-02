//
// Created by Fabi on 02.07.2020.
//

#include "ui.h"

#define FREQ_MAX_LEN 20
#define HISTORY_LEN 900

char *file_dir;

void set_ui_dir(char *dir) {
    file_dir = dir;
}

static FILE *open_file(const char *file, const char *mode) {
    char path[255];
    snprintf(path, sizeof(path), "%s%s", file_dir, file);

    return fopen(path, mode);
}

static void close_file(FILE *fp) {
    fclose(fp);
}

void load_config(struct config_data *config) {
    char buf[255];
    char *ptr;

    FILE *fp = open_file("calibration.csv", "r");

    // skip the header
    fscanf(fp, "%s", buf);

    // read arid value
    fscanf(fp, "%s", buf);
    config->arid = strtol(buf, &ptr, 10);

    // read humid value
    fscanf(fp, "%s", buf);
    config->humid = strtol(buf, &ptr, 10);

    // read millilitres
    fscanf(fp, "%s", buf);
    config->milliliters = strtol(buf, &ptr, 10);

    close_file(fp);
}

void send_freq_to_ui(double freq) {
    // + 1 because we need one free space for new value if history is already full
    char oldFreqValues[HISTORY_LEN + 1][FREQ_MAX_LEN];
    memset(oldFreqValues, 0, sizeof(oldFreqValues));
    int lineCount = 0;
    FILE *fp = 0;

    fp = open_file("test-hydro.csv", "r");
    // skip header
    fgets(oldFreqValues[lineCount], FREQ_MAX_LEN, fp);

    // read lines
    for (lineCount = 0; lineCount < HISTORY_LEN; lineCount++) {
        if (fgets(oldFreqValues[lineCount], FREQ_MAX_LEN, fp) == 0) {
            // got last line before buffer end
            break;
        }
    }
    close_file(fp);

    if (lineCount < HISTORY_LEN) {
        // just append new data because we have space in history
        fp = open_file("test-hydro.csv", "a");

        if (lineCount == 1) {
            // special case: empty file -> add header
            fprintf(fp, "Frequenz in Hz\n");
        }

        fprintf(fp, "%.0f\n", freq);
        close_file(fp);
    } else {
        // remove oldest history element and append new one
        char buf[FREQ_MAX_LEN];
        snprintf(buf, FREQ_MAX_LEN, "%.0f\n", freq);
        strncpy(oldFreqValues[HISTORY_LEN], buf, FREQ_MAX_LEN);

        fp = open_file("test-hydro.csv", "w");
        // write header
        fprintf(fp, "Frequenz in Hz\n");
        // i = 1 because we skip oldest element
        for (int i = 1; i < HISTORY_LEN + 1; i++) {
            fprintf(fp, "%s", oldFreqValues[i]);
        }
        close_file(fp);
    }

}

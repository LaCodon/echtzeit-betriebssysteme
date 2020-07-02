//
// Created by Fabi on 02.07.2020.
//

#ifndef GPIO_UI_H
#define GPIO_UI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct config_data {
    long int arid;
    long int humid;
    long int milliliters;
};

// Sets the directory where calibration.csv and test_hydro.csv can be found. Files must already exist! Don't forget to set trailing slash!
extern void set_ui_dir(char *dir);

// Loads the content of calibration.csv into *config
extern void load_config(struct config_data *config);

// Save freq into log with rotation
extern void send_freq_to_ui(double freq);

#endif //GPIO_UI_H

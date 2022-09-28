//
// Created by samuel on 23-9-22.
//

#ifndef FLATCAM_PCB_MAKER_GND_PADS_H
#define FLATCAM_PCB_MAKER_GND_PADS_H

#include "common.h"
#include "file_utils.h"

void gnd_pads_debug(const AppState *state);

bool remove_gnd_pads(const AppState *state, FILE *file, char **start_line);

void merge_connected_gnd_pads(AppState *state);

void calculate_location_of_pad(const AppState *state, const GndPad *pad, double *pad_x, double *pad_y);

double calculate_max_pad_radius(const AppState *state, GndPad *pad);

#endif //FLATCAM_PCB_MAKER_GND_PADS_H

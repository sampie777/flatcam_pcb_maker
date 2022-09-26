//
// Created by samuel on 23-9-22.
//

#ifndef FLATCAM_PCB_MAKER_GND_PADS_H
#define FLATCAM_PCB_MAKER_GND_PADS_H

#include "common.h"
#include "file_utils.h"

void gnd_pads_debug(AppState *state);

bool remove_gnd_pads(AppState *state, FILE *file, char **start_line);

#endif //FLATCAM_PCB_MAKER_GND_PADS_H

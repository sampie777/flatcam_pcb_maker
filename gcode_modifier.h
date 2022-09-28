//
// Created by samuel on 20-9-22.
//

#ifndef FLATCAM_PCB_MAKER_GCODE_MODIFIER_H
#define FLATCAM_PCB_MAKER_GCODE_MODIFIER_H

#include "common.h"

void gcode_modify(AppState *state);

int modify_silkscreen_file(AppState *state);

int modify_trace_file(AppState *state);

#endif //FLATCAM_PCB_MAKER_GCODE_MODIFIER_H

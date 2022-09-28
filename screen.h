//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_SCREEN_H
#define FLATCAM_PCB_MAKER_SCREEN_H


#include "common.h"

#define SCREEN_BUFFER_INIT {NULL, 0}

#define SCREEN_COLOR_RED "\x1b[31m"
#define SCREEN_COLOR_GREEN "\x1b[32m"
#define SCREEN_COLOR_YELLOW "\x1b[33m"
#define SCREEN_COLOR_RESET "\x1b[39m"

void screen_refresh(AppState *state);

#endif //FLATCAM_PCB_MAKER_SCREEN_H

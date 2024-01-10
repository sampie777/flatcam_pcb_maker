//
// Created by samuel on 22-9-22.
//

#ifndef FLATCAM_PCB_MAKER_EAGLE_BOARD_PARSER_H
#define FLATCAM_PCB_MAKER_EAGLE_BOARD_PARSER_H

#include "common.h"

int eagle_job_parse(AppState *state);

int eagle_profile_parse(AppState *state);

int eagle_board_parse(AppState *state);

#endif //FLATCAM_PCB_MAKER_EAGLE_BOARD_PARSER_H

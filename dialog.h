//
// Created by samuel on 19-9-22.
//

#ifndef FLATCAM_PCB_MAKER_DIALOG_H
#define FLATCAM_PCB_MAKER_DIALOG_H

#include "common.h"

void dialog_show(AppState *state, const char *title, const char *default_value, char *destination, int max_length);

void dialog_show_with_callback(AppState *state, const char *title, const char *default_value, char *destination, int max_length, void (*callback)(AppState *));

void dialog_confirm(AppState *state);

#endif //FLATCAM_PCB_MAKER_DIALOG_H

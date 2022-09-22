//
// Created by samuel on 19-9-22.
//

#ifndef FLATCAM_PCB_MAKER_DIALOG_H
#define FLATCAM_PCB_MAKER_DIALOG_H

#include "common.h"


void dialog_show_string(AppState *state, const char *title, const char *default_value, char *destination, int max_length);

void dialog_show_char(AppState *state, const char *title, char default_value, char *destination);

void dialog_show_string_with_callback(AppState *state, const char *title, const char *default_value, char *destination, int max_length, void (*callback)(AppState *));

void dialog_show_char_with_callback(AppState *state, const char *title, char default_value, char *destination, void (*callback)(AppState *));

void dialog_show_double_with_callback(AppState *state, const char *title, double default_value, double *destination, void (*callback)(AppState *));

void dialog_options_show_char_with_callback(AppState *state, const char *title, char default_value, char *destination, void (*callback)(AppState *), const char *options);

void dialog_confirm(AppState *state);

#endif //FLATCAM_PCB_MAKER_DIALOG_H

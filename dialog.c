//
// Created by samuel on 19-9-22.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dialog.h"
#include "utils.h"

void dialog_show_string(AppState *state, const char *title, const char *default_value, char *destination, int max_length) {
    state->dialog.show = 1;
    strcpy(state->dialog.title, title);
    strcpy(state->dialog.default_value, default_value);
    strcpy(state->dialog.value, default_value);
    state->dialog.destination_char = destination;
    state->dialog.max_length = max_length;
    state->dialog.type = 's';
    state->dialog.char_options[0] = '\0';
}

void dialog_show_char(AppState *state, const char *title, char default_value, char *destination) {
    state->dialog.show = 1;
    strcpy(state->dialog.title, title);
    memset(state->dialog.default_value, '\0', strlen(state->dialog.default_value));
    state->dialog.default_value[0] = default_value;
    state->dialog.value[0] = default_value;
    state->dialog.value[1] = '\0';
    state->dialog.destination_char = destination;
    state->dialog.max_length = 1;
    state->dialog.type = 'c';
    state->dialog.char_options[0] = '\0';
}

void dialog_show_double(AppState *state, const char *title, double default_value, double *destination) {
    state->dialog.show = 1;
    strcpy(state->dialog.title, title);
    sprintf(state->dialog.default_value, "%lf", default_value);
    auto_format_double_string(state->dialog.default_value);
    sprintf(state->dialog.value, "%lf", default_value);
    auto_format_double_string(state->dialog.value);
    state->dialog.destination_double = destination;
    state->dialog.max_length = max(10, strlen(state->dialog.value) + 2);
    state->dialog.type = 'f';
    state->dialog.char_options[0] = '\0';
}

void dialog_show_string_with_callback(AppState *state, const char *title, const char *default_value, char *destination, int max_length, void (*callback)(AppState *)) {
    dialog_show_string(state, title, default_value, destination, max_length);
    state->dialog.callback = (void (*)(void *)) callback;
}

void dialog_show_char_with_callback(AppState *state, const char *title, char default_value, char *destination, void (*callback)(AppState *)) {
    dialog_show_char(state, title, default_value, destination);
    state->dialog.callback = (void (*)(void *)) callback;
}

void dialog_show_double_with_callback(AppState *state, const char *title, double default_value, double *destination, void (*callback)(AppState *)) {
    dialog_show_double(state, title, default_value, destination);
    state->dialog.callback = (void (*)(void *)) callback;
}

void dialog_confirm(AppState *state) {
    if (strlen(state->dialog.value) > 0) {
        if (state->dialog.type == 's') {
            strcpy(state->dialog.destination_char, state->dialog.value);
        } else if (state->dialog.type == 'c') {
            *state->dialog.destination_char = state->dialog.value[0];
        } else if (state->dialog.type == 'f') {
            *state->dialog.destination_double = strtod(state->dialog.value, NULL);
        }
    }
    state->dialog.show = 0;
    state->dialog.callback(state);
}

void dialog_options_show_char_with_callback(AppState *state, const char *title, char default_value, char *destination, void (*callback)(AppState *), const char *options) {
    dialog_show_char_with_callback(state, title, default_value, destination, callback);
    strcpy(state->dialog.char_options, options);
    state->dialog.char_options[31] = '\0';

    // Set default selection
    for (int i = 0; i < strlen(state->dialog.char_options); i++) {
        if (state->dialog.char_options[i] == default_value) {
            state->dialog_selection = i;
            break;
        }
    }
}

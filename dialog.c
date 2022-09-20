//
// Created by samuel on 19-9-22.
//

#include <string.h>
#include "dialog.h"

void dialog_show_string(AppState *state, const char *title, const char *default_value, char *destination, int max_length) {
    state->dialog.show = 1;
    strcpy(state->dialog.title, title);
    strcpy(state->dialog.default_value, default_value);
    strcpy(state->dialog.value, default_value);
    state->dialog.destination = destination;
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
    state->dialog.destination = destination;
    state->dialog.max_length = 1;
    state->dialog.type = 'c';
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

void dialog_confirm(AppState *state) {
    if (strlen(state->dialog.value) > 0) {
        if (state->dialog.type == 's') {
            strcpy(state->dialog.destination, state->dialog.value);
        } else if (state->dialog.type == 'c') {
            *state->dialog.destination = state->dialog.value[0];
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

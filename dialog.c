//
// Created by samuel on 19-9-22.
//

#include <string.h>
#include "dialog.h"

void dialog_show(AppState *state, const char *title, const char *default_value, char *destination, int max_length) {
    state->dialog.show = 1;
    strcpy(state->dialog.title, title);
    strcpy(state->dialog.default_value, default_value);
    memset(state->dialog.value, '\0', strlen(state->dialog.value));
    state->dialog.destination = destination;
    state->dialog.max_length = max_length;
}

void dialog_show_with_callback(AppState *state, const char *title, const char *default_value, char *destination, int max_length, void (*callback)(AppState *)) {
    dialog_show(state, title, default_value, destination, max_length);
    state->dialog.callback = (void (*)(void *)) callback;
}

void dialog_confirm(AppState *state) {
    if (strlen(state->dialog.value) > 0) {
        strcpy(state->dialog.destination, state->dialog.value);
    }
    state->dialog.show = 0;
    state->dialog.callback(state);
}

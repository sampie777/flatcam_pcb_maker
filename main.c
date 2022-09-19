#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "terminal_utils.h"
#include "screen.h"
#include "file_utils.h"
#include "utils.h"
#include "dialog.h"
#include "flatcam_generator.h"

void selection_increase(AppState *state, int value) {
    switch (state->screen) {
        case SCREEN_SELECT_PROJECT:
            state->project_selection += value;
            break;
        case SCREEN_SELECT_ACTION:
            state->action_selection += value;
            break;
        case SCREEN_GENERATE_FLATCAM:
            state->flatcam_option_selection += value;
            break;
    }
}

void flatcam_screen_dialog_callback(AppState *state) {
    state->flatcam_option_selection++;
}

void confirm_selection(AppState *state) {
    if (state->dialog.show) {
        dialog_confirm(state);
        return;
    }

    switch (state->screen) {
        case SCREEN_SELECT_PROJECT: {
            size_t size = strlen(state->projects[state->project_selection]) * sizeof(char);
            state->project = malloc(size + 1);
            strcpy(state->project, state->projects[state->project_selection]);
            state->project[size] = '\0';
            state->screen = SCREEN_SELECT_ACTION;
            break;
        }
        case SCREEN_SELECT_ACTION: {
            switch (state->action_selection) {
                case ACTION_GENERATE_FLATCAM_COMMANDS:
                    state->screen = SCREEN_GENERATE_FLATCAM;
                    state->flatcam_option_selection = FLATCAM_BUTTON_GENERATE;
                    break;
                case ACTION_SHOW_CHECKLIST:
                    state->screen = SCREEN_SHOW_CHECKLIST;
                    break;
            }
            break;
        }
        case SCREEN_SHOW_CHECKLIST: {
            state->screen = SCREEN_SELECT_ACTION;
            break;
        }
        case SCREEN_GENERATE_FLATCAM: {
            switch (state->flatcam_option_selection) {
                case FLATCAM_COPPER_LAYER:
                    dialog_show_with_callback(state, "Copper layer [T,B]", state->flatcam_options.traces, &(state->flatcam_options.traces[0]), 1, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_MIRROR:
                    dialog_show_with_callback(state, "Mirror [Y,N]", state->flatcam_options.mirror, &(state->flatcam_options.mirror[0]), 1, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_OFFSET_X:
                    dialog_show_with_callback(state, "Offset X", state->flatcam_options.offset_x, &(state->flatcam_options.offset_x[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_OFFSET_Y:
                    dialog_show_with_callback(state, "Offset Y", state->flatcam_options.offset_y, &(state->flatcam_options.offset_y[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_DIA_WIDTH:
                    dialog_show_with_callback(state, "Dia width", state->flatcam_options.dia_width, &(state->flatcam_options.dia_width[0]), 9, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_FEEDRATE:
                    dialog_show_with_callback(state, "Feedrate", state->flatcam_options.feedrate_etch, &(state->flatcam_options.feedrate_etch[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_SILKSCREEN_TOP:
                    dialog_show_with_callback(state, "Silkscreen top [Y,N]", state->flatcam_options.silkscreen_top, &(state->flatcam_options.silkscreen_top[0]), 1, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_SILKSCREEN_BOTTOM:
                    dialog_show_with_callback(state, "Silkscreen bottom [Y,N]", state->flatcam_options.silkscreen_bottom, &(state->flatcam_options.silkscreen_bottom[0]), 1, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_BUTTON_GENERATE:
                    flatcam_generate(state);
                    state->flatcam_option_selection = FLATCAM_BUTTON_BACK;
                    break;
                case FLATCAM_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_ACTION;
                    break;
            }
            break;
        }
    }
}

void editorProcessKeypress(AppState *state) {
    int c = editorReadKey();

    switch (c) {
        case '\r':  // ENTER key
            confirm_selection(state);
            break;
        case CTRL_KEY('q'):
            clearScreen();
            exit(0);
        case ARROW_UP:
            selection_increase(state, -1);
            break;
        case ARROW_LEFT:
        case ARROW_DOWN:
            selection_increase(state, 1);
            break;
        case ARROW_RIGHT:
            break;
        case BACKSPACE:
            if (state->dialog.show) {
                int length = strlen(state->dialog.value);
                if (length > 0) {
                    state->dialog.value[length - 1] = '\0';
                }
            }
            break;
        case CTRL_KEY('h'):
        case DELETE_KEY:
            break;
        case CTRL_KEY('l'):
        case '\x1b': {   // ESCAPE key
            state->dialog.show = 0;
            break;
        }
        default:
            if (state->dialog.show) {
                int length = strlen(state->dialog.value);
                if (length < 63 && length < state->dialog.max_length) {
                    state->dialog.value[length] = (char) c;
                    state->dialog.value[length + 1] = '\0';
                }
            }
            break;
    }
}

void app_control(AppState *state) {
    state->project_selection = bound(state->project_selection, 0, state->projects_count - 1);
    state->action_selection = bound(state->action_selection, 0, ACTION_MAX_VALUE - 1);
    state->flatcam_option_selection = bound(state->flatcam_option_selection, 0, FLATCAM_MAX_VALUE - 1);
}

int main() {
    AppState state = {
            .flatcam_options.traces = "T",
            .flatcam_options.mirror = "Y",
            .flatcam_options.offset_x = "20",
            .flatcam_options.offset_y = "29",
            .flatcam_options.dia_width = "0.20188",
            .flatcam_options.feedrate_etch = "1400",
            .flatcam_options.silkscreen_top = "N",
            .flatcam_options.silkscreen_bottom = "N",
    };

    enableRawMode();

    if (getWindowSize(&state.row_count, &state.column_count) == -1)
        die("Failed to get window size");


    files_get_all_projects(PROJECTS_PATH, &state.projects, &state.projects_count);

    while (1) {
        app_control(&state);
        screen_refresh(&state);
        editorProcessKeypress(&state);
    }

    return 0;
}

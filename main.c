#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include "common.h"
#include "terminal_utils.h"
#include "screen.h"
#include "file_utils.h"
#include "utils.h"
#include "dialog.h"
#include "flatcam_generator.h"
#include "gcode_modifier.h"
#include "checklist.h"
#include "eagle_board_parser.h"

void selection_increase(AppState *state, int value) {
    if (state->dialog.show) {
        state->dialog_selection += value;
        return;
    }

    switch (state->screen) {
        case SCREEN_SELECT_PROJECT:
            state->project_selection += value;
            break;
        case SCREEN_SELECT_ACTION:
            state->action_selection += value;
            break;
        case SCREEN_GENERATE_FLATCAM:
            state->flatcam_option_selection += value;

            if (state->flatcam_option_selection == FLATCAM_SILKSCREEN_MIRROR && state->flatcam_options.silkscreen_top == 'N' && state->flatcam_options.silkscreen_bottom == 'N') {
                selection_increase(state, value);
            }
            break;
        case SCREEN_SHOW_CHECKLIST:
            state->checklist_selection += value;
            break;
        default:
            break;
    }
}

void selection_set(AppState *state, int value) {
    if (state->dialog.show) {
        state->dialog_selection = value;
        return;
    }

    switch (state->screen) {
        case SCREEN_SELECT_PROJECT:
            state->project_selection = value;
            break;
        case SCREEN_SELECT_ACTION:
            state->action_selection = value;
            break;
        case SCREEN_GENERATE_FLATCAM:
            state->flatcam_option_selection = value;
            break;
        default:
            break;
    }
}

void flatcam_screen_dialog_callback(AppState *state) {
    if (state->flatcam_option_selection == FLATCAM_COPPER_LAYER) {
        if (toupper(state->flatcam_options.traces) == 'T') {
            state->flatcam_options.mirror = 'N';
        } else if (toupper(state->flatcam_options.traces) == 'B') {
            state->flatcam_options.mirror = 'Y';
        }
    }
    selection_increase(state, 1);
}

void confirm_selection(AppState *state) {
    if (state->dialog.show) {
        dialog_confirm(state);
        return;
    }

    switch (state->screen) {
        case SCREEN_SELECT_PROJECT: {
            if (state->project_selection == state->projects_count) {
                // Exit program
                clearScreen();
                exit(0);
            }
            size_t size = strlen(state->projects[state->project_selection]) * sizeof(char);
            state->project = malloc(size + 1);
            strcpy(state->project, state->projects[state->project_selection]);
            state->screen = SCREEN_SELECT_ACTION;
            state->action_selection = 0;
            break;
        }
        case SCREEN_SELECT_ACTION: {
            switch (state->action_selection) {
                case ACTION_GENERATE_FLATCAM_COMMANDS:
                    state->screen = SCREEN_GENERATE_FLATCAM;
                    state->flatcam_option_selection = FLATCAM_BUTTON_GENERATE;
                    break;
                case ACTION_MODIFY_GCODE:
                    gcode_modify(state);
                    break;
                case ACTION_SHOW_CHECKLIST:
                    state->screen = SCREEN_SHOW_CHECKLIST;
                    state->checklist_check_position = 0;
                    state->checklist_selection = CHECKLIST_NEXT_CHECK;
                    break;
                case ACTION_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_PROJECT;
                    break;
            }
            break;
        }
        case SCREEN_SHOW_CHECKLIST: {
            if (state->checklist_selection == CHECKLIST_BUTTON_BACK) {
                state->screen = SCREEN_SELECT_ACTION;
            } else {
                state->checklist_check_position++;
            }
            break;
        }
        case SCREEN_GENERATE_FLATCAM: {
            switch (state->flatcam_option_selection) {
                case FLATCAM_COPPER_LAYER:
                    dialog_options_show_char_with_callback(state, "Copper layer", state->flatcam_options.traces, &(state->flatcam_options.traces), flatcam_screen_dialog_callback, "TB");
                    break;
                case FLATCAM_MIRROR:
                    dialog_options_show_char_with_callback(state, "Mirror", state->flatcam_options.mirror, &(state->flatcam_options.mirror), flatcam_screen_dialog_callback, "YN");
                    break;
                case FLATCAM_OFFSET_X:
                    dialog_show_double_with_callback(state, "Offset X", state->flatcam_options.offset_x, &(state->flatcam_options.offset_x), flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_OFFSET_Y:
                    dialog_show_double_with_callback(state, "Offset Y", state->flatcam_options.offset_y, &(state->flatcam_options.offset_y), flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_DIA_WIDTH:
                    dialog_show_string_with_callback(state, "Dia width", state->flatcam_options.dia_width, &(state->flatcam_options.dia_width[0]), 9, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_FEEDRATE:
                    dialog_show_string_with_callback(state, "Feedrate", state->flatcam_options.feedrate_etch, &(state->flatcam_options.feedrate_etch[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_ITERATIONS:
                    dialog_show_string_with_callback(state, "Iterations", state->flatcam_options.iterations, &(state->flatcam_options.iterations[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_SILKSCREEN_TOP:
                    dialog_options_show_char_with_callback(state, "Silkscreen top", state->flatcam_options.silkscreen_top, &(state->flatcam_options.silkscreen_top), flatcam_screen_dialog_callback, "YN");
                    break;
                case FLATCAM_SILKSCREEN_BOTTOM:
                    dialog_options_show_char_with_callback(state, "Silkscreen bottom", state->flatcam_options.silkscreen_bottom, &(state->flatcam_options.silkscreen_bottom), flatcam_screen_dialog_callback, "YN");
                    break;
                case FLATCAM_SILKSCREEN_MIRROR:
                    dialog_options_show_char_with_callback(state, "Mirror silkscreen", state->flatcam_options.silkscreen_mirror, &(state->flatcam_options.silkscreen_mirror), flatcam_screen_dialog_callback, "YN");
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
                size_t length = strlen(state->dialog.value);
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
            if (state->dialog.show && strlen(state->dialog.char_options) == 0) {
                size_t length = strlen(state->dialog.value);
                if (length < 63 && length < state->dialog.max_length) {
                    state->dialog.value[length] = (char) c;
                    state->dialog.value[length + 1] = '\0';
                }
            } else if (c >= 48 && c < 58) {
                selection_set(state, c - 48);
            }
            break;
    }
}

void app_control(AppState *state) {
    static time_t status_message_start_time = 0;

    if (state->checklist_check_position < checklist_length && checklist_checks[state->checklist_check_position][0] == '-') {
        state->checklist_check_position++;
    }
    if (state->checklist_check_position >= checklist_length) {
        state->checklist_selection = CHECKLIST_BUTTON_BACK;
    }

    state->project_selection = bound_int(state->project_selection, 0, state->projects_count, true);
    state->action_selection = bound_int(state->action_selection, 0, ACTION_MAX_VALUE - 1, true);
    state->flatcam_option_selection = bound_int(state->flatcam_option_selection, 0, FLATCAM_MAX_VALUE - 1, true);
    state->checklist_selection = bound_int(state->checklist_selection, 0, CHECKLIST_MAX_VALUE - 1, true);
    state->checklist_check_position = bound_int(state->checklist_check_position, 0, checklist_length, false);
    state->dialog_selection = bound_int(state->dialog_selection, 0, (int) strlen(state->dialog.char_options) - 1, true);
    if (strlen(state->dialog.char_options) > 0 && state->dialog.type == 'c') {
        state->dialog.value[0] = state->dialog.char_options[state->dialog_selection];
    }

    state->flatcam_options.traces = (char) toupper(state->flatcam_options.traces);
    state->flatcam_options.mirror = (char) toupper(state->flatcam_options.mirror);
    state->flatcam_options.silkscreen_top = (char) toupper(state->flatcam_options.silkscreen_top);
    state->flatcam_options.silkscreen_bottom = (char) toupper(state->flatcam_options.silkscreen_bottom);
    state->flatcam_options.silkscreen_mirror = (char) toupper(state->flatcam_options.silkscreen_mirror);

    if (strlen(state->status_message) != 0) {
        if (status_message_start_time == 0) {
            status_message_start_time = time(NULL);
        } else if (time(NULL) - status_message_start_time > 5) {
            state->status_message[0] = '\0';
            status_message_start_time = 0;
        }
    }
}

int main() {
    AppState state = {
            .flatcam_options.traces = 'B',
            .flatcam_options.mirror = 'Y',
            .flatcam_options.offset_x = 20,
            .flatcam_options.offset_y = 29,
            .flatcam_options.dia_width = "0.20188",
            .flatcam_options.feedrate_etch = "1400",
            .flatcam_options.iterations = "10",
            .flatcam_options.silkscreen_top = 'N',
            .flatcam_options.silkscreen_bottom = 'N',
            .flatcam_options.silkscreen_mirror = 'N',
    };
    state.status_message[0] = '\0';
    state.eagle_board = NULL;

    char projects_path[64];
#ifndef PROJECTS_PATH
    sprintf(projects_path, "%s/Documents/EAGLE/projects", getenv("USERPROFILE"));
#else
    sprintf(projects_path, "%s", PROJECTS_PATH);
#endif
    string_replace(projects_path, '\\', '/');
    state.projects_path = malloc(strlen(projects_path) + 1);
    strcpy(state.projects_path, projects_path);

    enableRawMode();

    if (getWindowSize(&state.row_count, &state.column_count) == -1)
        die("Failed to get window size");


    files_get_all_projects(state.projects_path, &state.projects, &state.projects_count);

    while (1) {
        app_control(&state);
        screen_refresh(&state);
        editorProcessKeypress(&state);
    }

    return 0;
}

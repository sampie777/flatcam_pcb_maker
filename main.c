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
#include "return_codes.h"
#include "gnd_pads.h"
#include "bed_leveling.h"

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
        case SCREEN_MODIFY_GCODE:
            state->modify_gcode_selection += value;
            break;
        case SCREEN_SHOW_CHECKLIST:
            state->checklist_selection += value;
            break;
        case SCREEN_PRINTER_LEVELING:
            state->printer_leveling_selection += value;
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
        case SCREEN_MODIFY_GCODE:
            state->modify_gcode_selection = value;
            break;
        default:
            break;
    }
}

void flatcam_screen_dialog_callback(AppState *state) {
    selection_increase(state, 1);
}

void free_eagle_board(AppState *state) {
    if (state->eagle_board == NULL) return;
    free(state->eagle_board->pads);
    free(state->eagle_board);
    state->eagle_board = NULL;
}

void on_project_selected(AppState *state) {
    strcpy(state->status_message, "");
    size_t size = strlen(state->projects[state->project_selection]);
    state->project = malloc(size + 1);
    strcpy(state->project, state->projects[state->project_selection]);

    free_eagle_board(state);
    eagle_profile_parse(state);
    if (eagle_job_parse(state) == RESULT_OK) {
        if (eagle_board_parse(state) == RESULT_OK) {
            merge_connected_gnd_pads(state);
        }
    }

    // Prefill measure points as a suggestion
    state->printer.measure0.x = state->flatcam_options.offset_x + 1;
    state->printer.measure0.y = state->flatcam_options.offset_y + 2;
    state->printer.measure1.y = state->printer.measure0.y;

    if (state->eagle_board != NULL) {
        state->printer.measure1.x = state->flatcam_options.offset_x + state->eagle_board->width;
        state->printer.measure2.x = state->flatcam_options.offset_x + state->eagle_board->width;
        state->printer.measure2.y = state->flatcam_options.offset_y + state->eagle_board->height;
    }
}

void toggle_char(char *source, char *selection) {
    for (int i = 0; i < strlen(selection); i++) {
        if (*source != selection[i]) continue;
        if (i == strlen(selection) - 1)
            *source = selection[0];
        else
            *source = selection[i + 1];
        return;
    }
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
            state->screen = SCREEN_SELECT_ACTION;
            state->action_selection = 0;
            on_project_selected(state);
            break;
        }
        case SCREEN_SELECT_ACTION: {
            switch (state->action_selection) {
                case ACTION_GENERATE_FLATCAM_COMMANDS:
                    state->screen = SCREEN_GENERATE_FLATCAM;
                    state->flatcam_option_selection = FLATCAM_BUTTON_GENERATE;
                    break;
                case ACTION_MODIFY_GCODE:
                    state->screen = SCREEN_MODIFY_GCODE;
                    gcode_modify(state);
                    break;
                case ACTION_SHOW_CHECKLIST:
                    state->screen = SCREEN_SHOW_CHECKLIST;
                    state->checklist_check_position = 0;
                    state->checklist_selection = CHECKLIST_NEXT_CHECK;
                    break;
                case ACTION_PRINTER_LEVELING:
                    state->screen = SCREEN_PRINTER_LEVELING;
                    state->printer_leveling_selection = 0;
                    break;
                case ACTION_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_PROJECT;
                    break;
                default:
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
        case SCREEN_PRINTER_LEVELING: {
            switch (state->printer_leveling_selection) {
                case PRINTER_LEVELING_MEASURE0_INPUT_X:
                    dialog_show_double_with_callback(state, "Point 0: X", state->printer.measure0.x, &(state->printer.measure0.x), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE0_INPUT_Y:
                    dialog_show_double_with_callback(state, "Point 0: Y", state->printer.measure0.y, &(state->printer.measure0.y), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE0_INPUT_Z:
                    dialog_show_double_with_callback(state, "Point 0: Z", state->printer.measure0.z, &(state->printer.measure0.z), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE1_INPUT_X:
                    dialog_show_double_with_callback(state, "Point 1: X", state->printer.measure1.x, &(state->printer.measure1.x), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE1_INPUT_Y:
                    dialog_show_double_with_callback(state, "Point 1: Y", state->printer.measure1.y, &(state->printer.measure1.y), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE1_INPUT_Z:
                    dialog_show_double_with_callback(state, "Point 1: Z", state->printer.measure1.z, &(state->printer.measure1.z), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE2_INPUT_X:
                    dialog_show_double_with_callback(state, "Point 2: X", state->printer.measure2.x, &(state->printer.measure2.x), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE2_INPUT_Y:
                    dialog_show_double_with_callback(state, "Point 2: Y", state->printer.measure2.y, &(state->printer.measure2.y), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_MEASURE2_INPUT_Z:
                    dialog_show_double_with_callback(state, "Point 2: Z", state->printer.measure2.z, &(state->printer.measure2.z), flatcam_screen_dialog_callback, true);
                    break;
                case PRINTER_LEVELING_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_ACTION;
                    break;
                default:
                    break;
            }
            break;
        }
        case SCREEN_GENERATE_FLATCAM: {
            switch (state->flatcam_option_selection) {
                case FLATCAM_COPPER_LAYER:
                    toggle_char(&(state->flatcam_options.traces), "TB");
                    state->flatcam_options.mirror = state->flatcam_options.traces == 'T' ? 'N' : 'Y';
                    break;
                case FLATCAM_MIRROR:
                    toggle_char(&(state->flatcam_options.mirror), "YN");
                    break;
                case FLATCAM_CUTOUT_PROFILE:
                    toggle_char(&(state->flatcam_options.cutout_profile), "YN");
                    break;
                case FLATCAM_OFFSET_X:
                    dialog_show_double_with_callback(state, "Offset X", state->flatcam_options.offset_x, &(state->flatcam_options.offset_x), flatcam_screen_dialog_callback, false);
                    break;
                case FLATCAM_OFFSET_Y:
                    dialog_show_double_with_callback(state, "Offset Y", state->flatcam_options.offset_y, &(state->flatcam_options.offset_y), flatcam_screen_dialog_callback, false);
                    break;
                case FLATCAM_DIA_WIDTH:
                    dialog_show_double_with_callback(state, "Dia width", state->flatcam_options.dia_width, &(state->flatcam_options.dia_width), flatcam_screen_dialog_callback, false);
                    break;
                case FLATCAM_FEEDRATE:
                    dialog_show_string_with_callback(state, "Feedrate", state->flatcam_options.feedrate_etch, &(state->flatcam_options.feedrate_etch[0]), 7, flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_ITERATIONS:
                    dialog_show_int_with_callback(state, "Iterations", state->flatcam_options.iterations, &(state->flatcam_options.iterations), flatcam_screen_dialog_callback);
                    break;
                case FLATCAM_REMOVE_GND_PADS:
                    toggle_char(&(state->flatcam_options.remove_gnd_pads), "YN");
                    break;
                case FLATCAM_SILKSCREEN_TOP:
                    toggle_char(&(state->flatcam_options.silkscreen_top), "YN");
                    break;
                case FLATCAM_SILKSCREEN_BOTTOM:
                    toggle_char(&(state->flatcam_options.silkscreen_bottom), "YN");
                    break;
                case FLATCAM_SILKSCREEN_MIRROR:
                    toggle_char(&(state->flatcam_options.silkscreen_mirror), "YN");
                    break;
                case FLATCAM_BUTTON_GENERATE:
                    flatcam_generate(state);
                    state->flatcam_option_selection = FLATCAM_BUTTON_BACK;
                    break;
                case FLATCAM_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_ACTION;
                    break;
                default:
                    break;
            }
            break;
        }
        case SCREEN_MODIFY_GCODE:
            switch (state->modify_gcode_selection) {
                case MODIFY_GCODE_BUTTON_BACK:
                    state->screen = SCREEN_SELECT_ACTION;
                    break;
                case MODIFY_GCODE_OPEN_FILES: {
                    char buffer[256];
                    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM/", state->projects_path, state->project);
                    open_folder(buffer);
                    break;
                }
                default:
                    break;
            }
            break;
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
            if (state->screen == SCREEN_PRINTER_LEVELING) {
                if (state->printer_leveling_selection >= PRINTER_LEVELING_MEASURE0_INPUT_X && state->printer_leveling_selection <= PRINTER_LEVELING_MEASURE0_INPUT_Z) {
                    state->printer_leveling_selection = -1;
                } else {
                    selection_increase(state, -3);
                }
            } else {
                selection_increase(state, -1);
            }
            break;
        case ARROW_LEFT:
            selection_increase(state, -1);
            break;
        case ARROW_DOWN:
            if (state->screen == SCREEN_PRINTER_LEVELING) {
                if (state->printer_leveling_selection >= PRINTER_LEVELING_MEASURE2_INPUT_X && state->printer_leveling_selection <= PRINTER_LEVELING_MEASURE2_INPUT_Z) {
                    state->printer_leveling_selection = PRINTER_LEVELING_BUTTON_BACK;
                } else if (state->printer_leveling_selection == PRINTER_LEVELING_BUTTON_BACK) {
                    state->printer_leveling_selection++;
                } else {
                    selection_increase(state, 3);
                }
            } else {
                selection_increase(state, 1);
            }
            break;
        case ARROW_RIGHT:
            selection_increase(state, 1);
            break;
        case BACKSPACE:
            if (state->dialog.show) {
                size_t length = strlen(state->dialog.value);
                if (length > 0) {
                    state->dialog.value[length - 1] = '\0';
                }
            } else {
                switch (state->screen) {
                    case SCREEN_SELECT_ACTION:
                        state->screen = SCREEN_SELECT_PROJECT;
                        break;
                    case SCREEN_GENERATE_FLATCAM:
                    case SCREEN_MODIFY_GCODE:
                    case SCREEN_SHOW_CHECKLIST:
                    case SCREEN_PRINTER_LEVELING:
                        state->screen = SCREEN_SELECT_ACTION;
                        break;
                    default:
                        break;
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
    state->modify_gcode_selection = bound_int(state->modify_gcode_selection, 0, MODIFY_GCODE_MAX_VALUE - 1, true);
    state->checklist_selection = bound_int(state->checklist_selection, 0, CHECKLIST_MAX_VALUE - 1, true);
    state->checklist_check_position = bound_int(state->checklist_check_position, 0, checklist_length, false);
    state->printer_leveling_selection = bound_int(state->printer_leveling_selection, 0, PRINTER_LEVELING_MAX_VALUE - 1, true);
    state->dialog_selection = bound_int(state->dialog_selection, 0, (int) strlen(state->dialog.char_options) - 1, true);
    if (strlen(state->dialog.char_options) > 0 && state->dialog.type == 'c') {
        state->dialog.value[0] = state->dialog.char_options[state->dialog_selection];
    }

    state->flatcam_options.traces = (char) toupper(state->flatcam_options.traces);
    state->flatcam_options.mirror = (char) toupper(state->flatcam_options.mirror);
    state->flatcam_options.cutout_profile = (char) toupper(state->flatcam_options.cutout_profile);
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

    if (state->screen == SCREEN_PRINTER_LEVELING) {
        bed_leveling_calculate(state);
    }
}

int main() {
    AppState state = {
            .flatcam_options.traces = 'B',
            .flatcam_options.mirror = 'Y',
            .flatcam_options.cutout_profile = 'Y',
            .flatcam_options.offset_x = 12,
            .flatcam_options.offset_y = 16,
            .flatcam_options.dia_width = 0.20188,
            .flatcam_options.feedrate_etch = "1400",
            .flatcam_options.iterations = 8,
            .flatcam_options.remove_gnd_pads = 'Y',
            .flatcam_options.silkscreen_top = 'N',
            .flatcam_options.silkscreen_bottom = 'N',
            .flatcam_options.silkscreen_mirror = 'N',
            .eagle_board = NULL,
            .modify_results.messages = NULL,
            .printer.mesh_size = 4,
            .printer.mesh_x_min = 12.0,
            .printer.mesh_x_max = 205.0,
            .printer.mesh_y_min = 12.0,
            .printer.mesh_y_max = 205.0,
//            .printer.head_offset_x = -48.4,
//            .printer.head_offset_y = -12.8,
    };
    state.status_message[0] = '\0';

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

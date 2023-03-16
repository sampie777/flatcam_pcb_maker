//
// Created by samuel on 20-9-22.
//

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include "gcode_modifier.h"
#include "utils.h"
#include "return_codes.h"
#include "file_utils.h"
#include "gnd_pads.h"
#include "screen.h"

#define TEMP_FILE_NAME "tempfile"

#define SETTINGS_COMMENT_START "; [FPB]"
#define G_USE_MESH "M420 S1 ; Use mesh"
#define G_BEEP "M300 S500 P500 ; Beep"
#define G_BEEP_END "M300 S2000 P500 ; Beep end"
#define G_SPINDLE_ON "M03"
#define G_SPINDLE_OFF "M05"
#define G_DWELL_1MS "G4 P1"
#define G_PAUSE "G4 S20"
#define G_HOME_AXIS "G28"
#define G_LCD_MESSAGE "M117"

typedef enum {
    STATUS_MESSAGE_INFO,
    STATUS_MESSAGE_SUCCESS,
    STATUS_MESSAGE_WARNING,
    STATUS_MESSAGE_ERROR,
} StatusMessageType;

void gcode_add_status_message(AppState *state, StatusMessageType type, const char *message) {
    state->modify_results.messages = realloc(state->modify_results.messages, sizeof(char *) * (state->modify_results.message_count + 1));

    char *color_code = "";
    switch (type) {
        case STATUS_MESSAGE_SUCCESS:
            color_code = SCREEN_COLOR_GREEN;
            break;
        case STATUS_MESSAGE_WARNING:
            color_code = SCREEN_COLOR_YELLOW;
            break;
        case STATUS_MESSAGE_ERROR:
            color_code = SCREEN_COLOR_RED;
            break;
        default:
            break;
    }

    state->modify_results.messages[state->modify_results.message_count] = malloc(strlen(message) + 16);
    sprintf(state->modify_results.messages[state->modify_results.message_count], "   %s%s%s", color_code, message, SCREEN_COLOR_RESET);
    state->modify_results.message_count++;
}

void generate_settings_comment(AppState *state, char **out) {
    *out = malloc(1024);
    sprintf(*out,
            SETTINGS_COMMENT_START" file=%s\n"
            SETTINGS_COMMENT_START" width=%.4lf\n"
            SETTINGS_COMMENT_START" height=%.4lf\n"
            SETTINGS_COMMENT_START" min_x=%.4lf\n"
            SETTINGS_COMMENT_START" min_y=%.4lf\n"
            SETTINGS_COMMENT_START" max_x=%.4lf\n"
            SETTINGS_COMMENT_START" max_y=%.4lf\n"
            SETTINGS_COMMENT_START" traces=%c\n"
            SETTINGS_COMMENT_START" mirror=%c\n"
            SETTINGS_COMMENT_START" cutout_profile=%c\n"
            SETTINGS_COMMENT_START" offset_x=%lf\n"
            SETTINGS_COMMENT_START" offset_y=%lf\n"
            SETTINGS_COMMENT_START" dia_width=%lf\n"
            SETTINGS_COMMENT_START" feedrate_etch=%s\n"
            SETTINGS_COMMENT_START" iterations=%d\n"
            SETTINGS_COMMENT_START" remove_gnd_pads=%c\n"
            SETTINGS_COMMENT_START" silkscreen_top=%c\n"
            SETTINGS_COMMENT_START" silkscreen_bottom=%c\n"
            SETTINGS_COMMENT_START" silkscreen_mirror=%c\n",
            state->eagle_board == NULL ? "(null)" : state->eagle_board->name,
            state->eagle_board == NULL ? 0 : state->eagle_board->width,
            state->eagle_board == NULL ? 0 : state->eagle_board->height,
            state->eagle_board == NULL ? 0 : state->eagle_board->min_x,
            state->eagle_board == NULL ? 0 : state->eagle_board->min_y,
            state->eagle_board == NULL ? 0 : state->eagle_board->max_x,
            state->eagle_board == NULL ? 0 : state->eagle_board->max_y,
            state->flatcam_options.traces,
            state->flatcam_options.mirror,
            state->flatcam_options.cutout_profile,
            state->flatcam_options.offset_x,
            state->flatcam_options.offset_y,
            state->flatcam_options.dia_width,
            state->flatcam_options.feedrate_etch,
            state->flatcam_options.iterations,
            state->flatcam_options.remove_gnd_pads,
            state->flatcam_options.silkscreen_top,
            state->flatcam_options.silkscreen_bottom,
            state->flatcam_options.silkscreen_mirror
    );
}

int replace_file_with_tempfile(AppState *state, const char *filename) {
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM/%s", state->projects_path, state->project, filename);
    int result = copy_file(TEMP_FILE_NAME, buffer);
    if (result != RESULT_OK) {
        char buffer2[512];
        sprintf(buffer2, "   Failed to modify file %s.", buffer);
        gcode_add_status_message(state, STATUS_MESSAGE_ERROR, buffer2);
    }
    remove(TEMP_FILE_NAME);
    return result;
}

int open_files(AppState *state, const char *filename, FILE **input_file, FILE **output_file) {
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM/%s", state->projects_path, state->project, filename);
    *input_file = fopen(buffer, "r");
    if (*input_file == NULL) {
        char buffer2[512];
        sprintf(buffer2, "   Failed to open file %s.", buffer);
        gcode_add_status_message(state, STATUS_MESSAGE_ERROR, buffer2);
        return RESULT_FAILED;
    }

    if (output_file != NULL) {
        *output_file = fopen(TEMP_FILE_NAME, "w");
        if (*output_file == NULL) {
            gcode_add_status_message(state, STATUS_MESSAGE_ERROR, "   Failed to open temp file.");
            return RESULT_FAILED;
        }
    }
    return RESULT_OK;
}

int get_profile_bounds(AppState *state, double *min_x, double *min_y, double *max_x, double *max_y) {
    FILE *file;
    int result = open_files(state, TRACES_OUTPUT_FILE, &file, NULL);
    if (result != RESULT_OK) return result;

    *min_x = 1000;
    *min_y = 1000;
    *max_x = 0;
    *max_y = 0;

    bool at_least_one_found = false;
    int command;
    double x, y;
    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (!(starts_with(line, "G01 X") || starts_with(line, "G00 X"))) {
            continue;
        }

        if (sscanf(line, "G0%d X%lfY%lf", &command, &x, &y) != 3) {
            continue;
        }
        if (command > 1) continue;
        if (x == 0 && y == 0) continue;

        if (x < *min_x) *min_x = x;
        if (x > *max_x) *max_x = x;
        if (y < *min_y) *min_y = y;
        if (y > *max_y) *max_y = y;
        at_least_one_found = true;
    }

    fclose(file);

    if (!at_least_one_found) {
        gcode_add_status_message(state, STATUS_MESSAGE_ERROR, "   Failed to get profile min/max bounds.");
        return RESULT_FAILED;
    }
    return RESULT_OK;
}

bool limit_line_to_bounds(char **line, double min_x, double min_y, double max_x, double max_y, bool ignore_origin) {
    int command;
    double x, y;
    if (sscanf(*line, "G0%d X%lfY%lf", &command, &x, &y) != 3) {
        return false;
    }
    if (command > 1) return false;

    if (ignore_origin && x == 0 && y == 0) return false;
    if (x >= min_x && x <= max_x && y > min_y && y < max_y) return false;

    x = bound_double(x, min_x, max_x);
    y = bound_double(y, min_y, max_y);

    if (strlen(*line) < 31) {
        *line = malloc(32);
    }
    sprintf(*line, "G0%d X%.4lfY%.4lf", command, x, y);
    return true;
}

int modify_pre_drill_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, PRE_DRILLS_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        }

        fprintf(temp_file, "%s\n", line);
    }

    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, PRE_DRILLS_OUTPUT_FILE);
}

int modify_drill_file(AppState *state) {
    double min_x, min_y, max_x, max_y;
    int result = get_profile_bounds(state, &min_x, &min_y, &max_x, &max_y);
    if (result != RESULT_OK) return result;

    FILE *file, *temp_file;
    result = open_files(state, DRILLS_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool pause_print_present = false;
    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    bool removed_a_hole = false;
    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }
        if (starts_with(line, G_SPINDLE_ON) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        }
        if (starts_with(line, G_SPINDLE_ON) && !pause_print_present) {
            fprintf(temp_file, G_BEEP"\n"
                               G_LCD_MESSAGE" Set Z-offset to -3.0 mm\n"
                               G_PAUSE" ; Insert pause so use can set/check z-offset\n"
                               G_SPINDLE_ON" ; Empty commands so the printer has time to pause\n"
                               G_SPINDLE_ON"\n"
                               G_SPINDLE_ON"\n"
                               G_SPINDLE_ON"\n"
                               G_SPINDLE_ON"\n"
                               G_SPINDLE_ON"\n"
                               G_LCD_MESSAGE" Remove Z-stop!\n"
                               G_BEEP"\n");
            pause_print_present = true;
            continue;
        }

        if (strcmp(line, "G01 Z-3.0000") == 0) {
            if (!removed_a_hole) fprintf(temp_file, "G01 Z3.2000\nG01 Z0.0000 F10.0\n");
            continue;
        }
        if (strcmp(line, "G01 Z0") == 0) {
            if (!removed_a_hole) fprintf(temp_file, "G01 Z3.2 F50.0\n");
            continue;
        }
        if (strcmp(line, "G00 Z1.5000") == 0) {
            fprintf(temp_file, "G00 Z4.5000 F1000\n\n");
            continue;
        }

        if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_BEEP) == 0) {
            pause_print_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        } else if (starts_with(line, "G00 X") || starts_with(line, "G01 X")) {
            removed_a_hole = false;
            if (limit_line_to_bounds(&line, min_x, min_y, max_x, max_y, true)) {
                // Remove these holes completely
                removed_a_hole = true;
                fprintf(temp_file, "; Removed hole because it's out of bounds: %s\n", line);
                continue;
            }
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, DRILLS_OUTPUT_FILE);
}

int modify_check_holes_file(AppState *state) {
    double min_x, min_y, max_x, max_y;
    int result = get_profile_bounds(state, &min_x, &min_y, &max_x, &max_y);
    if (result != RESULT_OK) return result;

    FILE *file, *temp_file;
    result = open_files(state, DRILLS_CHECK_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool use_mesh_present = false;

    bool removed_a_hole = false;
    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }
        if (strcmp(line, "G01 Z0") == 0) {
            if (!removed_a_hole) fprintf(temp_file, G_PAUSE" ; Pause\n"G_BEEP"\n");
            continue;
        }
        if (strcmp(line, "G01 Z0.3000") == 0) {
            if (!removed_a_hole) fprintf(temp_file, "G01 Z0.3000 F100.0\n");
            continue;
        }
        if (strcmp(line, "G00 Z2.5000") == 0) {
            fprintf(temp_file, "G00 Z2.5000 F1000.0\n\n");
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (starts_with(line, "G00 X") || starts_with(line, "G01 X")) {
            removed_a_hole = false;
            if (limit_line_to_bounds(&line, min_x, min_y, max_x, max_y, true)) {
                // Remove these holes completely
                removed_a_hole = true;
                fprintf(temp_file, "; Removed hole because it's out of bounds: %s\n", line);
                continue;
            }
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, DRILLS_CHECK_OUTPUT_FILE);
}

int modify_check_holes_mirrored_file(AppState *state) {
    double min_x, min_y, max_x, max_y;
    int result = get_profile_bounds(state, &min_x, &min_y, &max_x, &max_y);
    if (result != RESULT_OK) return result;

    FILE *file, *temp_file;
    result = open_files(state, DRILLS_MIRRORED_CHECK_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool use_mesh_present = false;

    bool removed_a_hole = false;
    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }
        if (strcmp(line, "G01 Z0") == 0) {
            if (!removed_a_hole) fprintf(temp_file, G_PAUSE" ; Pause\n"G_BEEP"\n");
            continue;
        }
        if (strcmp(line, "G01 Z0.3000") == 0) {
            if (!removed_a_hole) fprintf(temp_file, "G01 Z0.3000 F100.0\n");
            continue;
        }
        if (strcmp(line, "G00 Z2.5000") == 0) {
            fprintf(temp_file, "G00 Z2.5000 F1000.0\n\n");
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (starts_with(line, "G00 X") || starts_with(line, "G01 X")) {
            removed_a_hole = false;
            if (limit_line_to_bounds(&line, min_x, min_y, max_x, max_y, true)) {
                // Remove these holes completely
                removed_a_hole = true;
                fprintf(temp_file, "; Removed hole because it's out of bounds: %s\n", line);
                continue;
            }
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, DRILLS_MIRRORED_CHECK_OUTPUT_FILE);
}

int modify_trace_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, TRACES_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool use_mesh_present = false;
    bool end_print_beep_present = false;
    bool has_been_checked_for_gnd_pad = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        } else if (state->flatcam_options.remove_gnd_pads == 'Y' && starts_with(line, "G01 X")) {
            if (!has_been_checked_for_gnd_pad)
                has_been_checked_for_gnd_pad = !remove_gnd_pads(state, file, &line);
        } else {
            has_been_checked_for_gnd_pad = false;
        }

        fprintf(temp_file, "%s\n", line);
    }

    // Show located GND pads in gcode
//    double pad_x;
//    double pad_y;
//    for (int i = 0; i < state->eagle_board->pad_count; i++) {
//        GndPad *pad = &(state->eagle_board->pads[i]);
//        calculate_location_of_pad(state, pad, &pad_x, &pad_y);
//        double radius = calculate_max_pad_radius(state, pad);
//
//        fprintf(temp_file, "G00 X%lfY%lf\n", pad_x, pad_y);
//        fprintf(temp_file, "G01 Z0.0000\n");
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x - radius, pad_y);
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x - radius, pad_y - radius);
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x + radius, pad_y - radius);
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x + radius, pad_y + radius);
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x - radius, pad_y + radius);
//        fprintf(temp_file, "G01 X%lfY%lf\n", pad_x - radius, pad_y - radius);
//        fprintf(temp_file, "G00 Z2.0000\n");
//    }

    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, TRACES_OUTPUT_FILE);
}

int modify_silkscreen_file(AppState *state) {
    double min_x, min_y, max_x, max_y;
    int result = get_profile_bounds(state, &min_x, &min_y, &max_x, &max_y);
    if (result != RESULT_OK) return result;

    if (min_x < state->flatcam_options.offset_x || min_y < state->flatcam_options.offset_y) {
        gcode_add_status_message(state, STATUS_MESSAGE_WARNING, "   Some traces are out of bounds!");
    }

    FILE *file, *temp_file;
    result = open_files(state, SILKSCREEN_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool settings_comment_present = false;
    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, SETTINGS_COMMENT_START)) continue;
        if (!settings_comment_present) {
            char *buffer;
            generate_settings_comment(state, &buffer);
            fprintf(temp_file, "%s", buffer);
            settings_comment_present = true;
        }

        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            if (state->printer.use_bed_leveling_mesh) {
                fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            } else {
                fprintf(temp_file, G_HOME_AXIS"\n");
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            if (!state->printer.use_bed_leveling_mesh) {
                continue;
            }
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        } else if (starts_with(line, "G00 X") || starts_with(line, "G01 X")) {
            limit_line_to_bounds(&line, min_x, min_y, max_x, max_y, true);
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, SILKSCREEN_OUTPUT_FILE);
}

int total_removed_gnd_pads(AppState *state) {
    int count = 0;
    for (int i = 0; i < state->eagle_board->pad_count; i++) {
        if (state->eagle_board->pads[i].has_been_removed)
            count++;
    }
    return count;
}

void free_result_messages(AppState *state) {
    while (state->modify_results.message_count > 0) {
        free(state->modify_results.messages[--state->modify_results.message_count]);
    }
}

void gcode_modify(AppState *state) {
    free_result_messages(state);

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Traces:");
    if (modify_trace_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");

    int removed_gnd_pads = total_removed_gnd_pads(state);
    if (state->flatcam_options.remove_gnd_pads == 'Y' && removed_gnd_pads != state->eagle_board->pad_count) {
        char buffer[256];
        sprintf(buffer, "   Only removed %d/%d GND pads.", removed_gnd_pads, state->eagle_board->pad_count);
        gcode_add_status_message(state, STATUS_MESSAGE_WARNING, buffer);
    }

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Pre-drills:");
    if (modify_pre_drill_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Check-holes:");
    if (modify_check_holes_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Drills:");
    if (modify_drill_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Check-holes mirrored:");
    if (modify_check_holes_mirrored_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");

    gcode_add_status_message(state, STATUS_MESSAGE_INFO, "Silkscreen:");
    if (modify_silkscreen_file(state) == RESULT_OK) gcode_add_status_message(state, STATUS_MESSAGE_SUCCESS, "   Modified.");
}

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

#define TEMP_FILE_NAME "tempfile"

#define G_USE_MESH "M420 S1 ; Use mesh"
#define G_BEEP "M300 S500 P500 ; Beep"
#define G_BEEP_END "M300 S2000 P500 ; Beep end"
#define G_SPINDLE_ON "M03"
#define G_SPINDLE_OFF "M05"
#define G_DWELL_1MS "G4 P1"
#define G_PAUSE "G4 S20"
#define G_HOME_AXIS "G28"
#define G_LCD_MESSAGE "M117"

int replace_file_with_tempfile(AppState *state, const char *filename) {
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM/%s", state->projects_path, state->project, filename);
    int result = copy_file(TEMP_FILE_NAME, buffer);
    if (result != RESULT_OK) {
        sprintf(state->status_message, "Failed to modify file %s", buffer);
    }
    remove(TEMP_FILE_NAME);
    return result;
}

int open_files(AppState *state, const char *filename, FILE **input_file, FILE **output_file) {
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM/%s", state->projects_path, state->project, filename);
    *input_file = fopen(buffer, "r");
    *output_file = fopen(TEMP_FILE_NAME, "w");

    if (*input_file == NULL) {
        sprintf(state->status_message, "Failed to open file %s", buffer);
        return RESULT_FAILED;
    }
    if (*output_file == NULL) {
        strcpy(state->status_message, "Failed to open temp file");
        return RESULT_FAILED;
    }
    return RESULT_OK;
}

int modify_drill_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, DRILLS_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool pause_print_present = false;
    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
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
                               G_BEEP"\n");
            pause_print_present = true;
            continue;
        }

        if (strcmp(line, "G01 Z-3.0000") == 0) {
            fprintf(temp_file, "G01 Z3.2000\nG01 Z0.0000 F10.0\n");
            continue;
        }
        if (strcmp(line, "G01 Z0") == 0) {
            fprintf(temp_file, "G01 Z3.2 F50.0\n");
            continue;
        }
        if (strcmp(line, "G00 Z1.5000") == 0) {
            fprintf(temp_file, "G00 Z4.5000 F1000\n\n");
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_BEEP) == 0) {
            pause_print_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, DRILLS_OUTPUT_FILE);
}

int modify_check_holes_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, DRILLS_CHECK_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool use_mesh_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }
        if (strcmp(line, "G01 Z0") == 0) {
            fprintf(temp_file, G_PAUSE" ; Pause\n"G_BEEP"\n");
            continue;
        }
        if (strcmp(line, "G01 Z0.3000") == 0) {
            fprintf(temp_file, "G01 Z0.3000 F100.0\n");
            continue;
        }
        if (strcmp(line, "G00 Z2.5000") == 0) {
            fprintf(temp_file, "G00 Z2.5000 F1000.0\n\n");
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            use_mesh_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            use_mesh_present = true;
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, DRILLS_CHECK_OUTPUT_FILE);
}

int modify_trace_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, TRACES_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, TRACES_OUTPUT_FILE);
}

int modify_silkscreen_file(AppState *state) {
    FILE *file, *temp_file;
    int result = open_files(state, SILKSCREEN_OUTPUT_FILE, &file, &temp_file);
    if (result != RESULT_OK) return result;

    bool use_mesh_present = false;
    bool end_print_beep_present = false;

    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "F")) {
            fprintf(temp_file, "G0 %s\n", line);
            continue;
        }

        if (starts_with(line, G_DWELL_1MS) && !use_mesh_present) {
            fprintf(temp_file, G_HOME_AXIS" ; Home axis before we can use mesh\n"G_USE_MESH"\n");
            use_mesh_present = true;
        } else if (strcmp(line, G_SPINDLE_OFF) == 0 && !end_print_beep_present) {
            fprintf(temp_file, G_BEEP_END"\n");
            end_print_beep_present = true;
        } else if (strcmp(line, G_USE_MESH) == 0) {
            use_mesh_present = true;
        } else if (strcmp(line, G_BEEP_END) == 0) {
            end_print_beep_present = true;
        }

        fprintf(temp_file, "%s\n", line);
    }
    if (line) free(line);

    fclose(temp_file);
    fclose(file);
    return replace_file_with_tempfile(state, SILKSCREEN_OUTPUT_FILE);
}

void gcode_modify(AppState *state) {
    int result = modify_trace_file(state);
    result |= modify_silkscreen_file(state);
    result |= modify_check_holes_file(state);
    result |= modify_drill_file(state);

    if (result == RESULT_OK) {
        strcpy(state->status_message, "Modified!");
    }
}

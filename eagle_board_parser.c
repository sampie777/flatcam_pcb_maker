//
// Created by samuel on 22-9-22.
//

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "eagle_board_parser.h"
#include "utils.h"
#include "return_codes.h"
#include "file_utils.h"

#define MIL_TO_MM_FACTOR 0.0254
#define SIGNAL_NAME "GND"

void strip_extra_chars(char *text) {
    while (strlen(text) > 0 && (text[strlen(text) - 1] == '\"' || text[strlen(text) - 1] == '>')) {
        text[strlen(text) - 1] = '\0';
    }
}

int open_file(AppState *state, const char *file_name, FILE **file) {
    *file = fopen(file_name, "r");
    if (*file == NULL) {
        sprintf(state->status_message, "Failed to open file %s", file_name);
        return RESULT_FAILED;
    }
    return RESULT_OK;
}

int read_file_designrules(FILE *file, EagleBoardProject *project) {
    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</designrules")) break;

        char *value = strstr(line, "value=\"");
        if (value == NULL) continue;

        // Strip value=" part
        value = &(value[7]);

        if (strstr(line, "\"rvPadTop\"") != NULL) {
            project->design_rules.pad_hole_to_mask_ratio = strtod(value, NULL);
        } else if (strstr(line, "\"rlMinPadTop\"") != NULL) {
            project->design_rules.pad_min_mask_diameter = strtod(value, NULL);
            if (strstr(value, "mil\"") != NULL) {
                project->design_rules.pad_min_mask_diameter *= MIL_TO_MM_FACTOR;
            }
        } else if (strstr(line, "\"rlMaxPadTop\"") != NULL) {
            project->design_rules.pad_max_mask_diameter = strtod(value, NULL);
            if (strstr(value, "mil\"") != NULL) {
                project->design_rules.pad_max_mask_diameter *= MIL_TO_MM_FACTOR;
            }
        } else if (strstr(line, "\"psElongationLong\"") != NULL) {
            project->design_rules.pad_shape_long_ratio = strtod(value, NULL) / 100.0;
        }
    }

    if (line) free(line);
    return result;
}

int read_file_gnd_signal(AppState *state, FILE *file, EagleBoardProject *project) {
    project->pad_count = 0;
    project->pads = NULL;

    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</signal>")) break;
        if (!starts_with(line, "<contactref ")) break;

        project->pad_count++;
        project->pads = realloc(project->pads, sizeof(GndPad) * project->pad_count);
        GndPad *pad = &(project->pads[project->pad_count - 1]);

        strtok(line, "\"");
        pad->name = strtok(NULL, "\"");
        strtok(NULL, "\"");
        pad->package_pad.name = strtok(NULL, "\"");
        pad->library = NULL;
        pad->package = NULL;
        pad->x = 0;
        pad->y = 0;
        pad->rotation = 0;

        if (pad->name == NULL || pad->package_pad.name == NULL) {
            strcpy(state->status_message, "Failed to get GND pad specifics");
            result = RESULT_FAILED;
            break;
        }
    }

    if (line) free(line);
    return result;
}

int read_file_elements(FILE *file, EagleBoardProject *project) {
    if (project->pad_count == 0) return RESULT_OK;

    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</elements>")) break;

        char name[64] = "", library[64] = "", package[64] = "", x_text[64] = "", y_text[64] = "", rotation_text[64] = "";
        bool inverted = strstr(line, "rot=\"MR") != NULL;

        char *part = strtok(line, " ");
        while ((part = strtok(NULL, " ")) != NULL) {
            sscanf(part, "name=\"%s\"", name);
            sscanf(part, "library=\"%s\"", library);
            sscanf(part, "package=\"%s\"", package);
            sscanf(part, "x=\"%s\"", x_text);
            sscanf(part, "y=\"%s\"", y_text);
            sscanf(part, "rot=\"R%s\"", rotation_text);
            sscanf(part, "rot=\"MR%s\"", rotation_text);
        }

        strip_extra_chars(name);
        strip_extra_chars(library);
        strip_extra_chars(package);
        strip_extra_chars(x_text);
        strip_extra_chars(y_text);
        strip_extra_chars(rotation_text);

        double x = strtod(x_text, NULL);
        double y = strtod(y_text, NULL);
        double rotation = strtod(rotation_text, NULL);

        for (int i = 0; i < project->pad_count; i++) {
            GndPad *pad = &(project->pads[i]);
            if (strcmp(name, pad->name) != 0) continue;

            pad->library = malloc(strlen(library) + 1);
            strcpy(pad->library, library);
            pad->package = malloc(strlen(package) + 1);
            strcpy(pad->package, package);

            pad->x = x;
            pad->y = y;
            pad->rotation = rotation;
            pad->inverted = inverted;
        }
    }

    if (line) free(line);
    return result;
}

int read_file_libraries(FILE *file, EagleBoardProject *project) {
    if (project->pad_count == 0) return RESULT_OK;

    bool description_started = false;
    char library_name[64];
    char package_name[64];

    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</libraries>")) break;
        if (starts_with(line, "<packages3d")) continue;
        if (starts_with(line, "<package3d")) continue;
        if (starts_with(line, "<packages")) continue;
        if (starts_with(line, "</packages>")) continue;
        if (starts_with(line, "<description")) description_started = true;
        if (strstr(line, "</description>") != NULL) {
            description_started = false;
            continue;
        }
        if (description_started) continue;
        if (starts_with(line, "</library>")) {
            library_name[0] = '\0';
            continue;
        }

        if (starts_with(line, "<library ")) {
            if (sscanf(line, "<library name=\"%s\"", library_name) != 1) {
                library_name[0] = '\0';
                printf("No name found for library: %s\n", line);
            }
            strip_extra_chars(library_name);
            continue;
        }

        if (library_name[0] == '\0') continue;

        if (starts_with(line, "<package ")) {
            if (sscanf(line, "<package name=\"%s\"", package_name) != 1) {
                package_name[0] = '\0';
                printf("No name found for package: %s\n", line);
            }
            while (strlen(package_name) > 0
                   && (package_name[strlen(package_name) - 1] == '\"'
                       || package_name[strlen(package_name) - 1] == '>')) {
                package_name[strlen(package_name) - 1] = '\0';
            }
            continue;
        }

        if (package_name[0] == '\0') continue;

        if (!starts_with(line, "<pad ")) continue;

        char pad_name[64], x_text[64], y_text[64], rotation_text[64], drill_text[64], diameter_text[64], shape_text[64];
        char *part = strtok(line, " ");
        while ((part = strtok(NULL, " ")) != NULL) {
            sscanf(part, "name=\"%s\"", pad_name);
            sscanf(part, "x=\"%s\"", x_text);
            sscanf(part, "y=\"%s\"", y_text);
            sscanf(part, "rot=\"R%s\"", rotation_text);
            sscanf(part, "rot=\"MR%s\"", rotation_text);
            sscanf(part, "drill=\"%s\"", drill_text);
            sscanf(part, "diameter=\"%s\"", diameter_text);
            sscanf(part, "shape=\"%s\"", shape_text);
        }

        strip_extra_chars(pad_name);
        strip_extra_chars(x_text);
        strip_extra_chars(y_text);
        strip_extra_chars(rotation_text);
        strip_extra_chars(drill_text);
        strip_extra_chars(diameter_text);
        strip_extra_chars(shape_text);

        double x = strtod(x_text, NULL);
        double y = strtod(y_text, NULL);
        double rotation = strtod(rotation_text, NULL);
        double drill = strtod(drill_text, NULL);
        double diameter = strtod(diameter_text, NULL);

        PadShape shape = SHAPE_ROUND;
        if (starts_with(shape_text, "square")) shape = SHAPE_SQUARE;
        else if (starts_with(shape_text, "long")) shape = SHAPE_LONG;
        else if (starts_with(shape_text, "octagon")) shape = SHAPE_OCTAGON;

        for (int i = 0; i < project->pad_count; i++) {
            GndPad *pad = &(project->pads[i]);
            if ((pad->library != NULL && strcmp(library_name, pad->library) != 0)
                || (pad->package != NULL && strcmp(package_name, pad->package) != 0)
                || strcmp(pad_name, pad->package_pad.name) != 0)
                continue;

            pad->package_pad.x = x;
            pad->package_pad.y = y;
            pad->package_pad.rotation = rotation;
            pad->package_pad.drill_size = drill;
            pad->package_pad.diameter = diameter;
            pad->package_pad.shape = shape;
        }
    }

    if (line) free(line);
    return result;
}

int read_board_file(AppState *state, FILE *file, EagleBoardProject *project) {
    char *line = NULL;
    int result;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "<designrules")) {
            result = read_file_designrules(file, project);
            if (result != RESULT_OK) break;
        } else if (starts_with(line, "<signal name=\""SIGNAL_NAME"\"")) {
            result = read_file_gnd_signal(state, file, project);
            if (result != RESULT_OK) break;
        }
    }

    if (result == RESULT_OK) {
        // Reread file from beginning
        fseek(file, 0, SEEK_SET);
        while (file_read_line(file, &line) == RESULT_OK) {
            if (starts_with(line, "<elements")) {
                result = read_file_elements(file, project);
                if (result != RESULT_OK) break;
            }
        }
    }

    if (result == RESULT_OK) {
        // Reread file from beginning
        fseek(file, 0, SEEK_SET);
        while (file_read_line(file, &line) == RESULT_OK) {
            if (starts_with(line, "<libraries")) {
                result = read_file_libraries(file, project);
                if (result != RESULT_OK) break;
            }
        }
    }
    if (line) free(line);

    if (result == RESULT_EMPTY) return RESULT_OK;
    return result;
}

int validate_project(AppState *state, EagleBoardProject *project) {
    char buffer[256];
    if (project->design_rules.pad_hole_to_mask_ratio < 0 ||
        project->design_rules.pad_min_mask_diameter < 0 ||
        project->design_rules.pad_max_mask_diameter < 0) {
        strcpy(state->status_message, "Failed to load design rules");
        return RESULT_FAILED;
    }
    for (int i = 0; i < project->pad_count; i++) {
        GndPad *pad = &(project->pads[i]);

        if (pad->name == NULL || pad->package_pad.name == NULL) {
            strcpy(state->status_message, "Failed to get GND pad specifics");
            return RESULT_FAILED;
        }
        if (pad->library == NULL || strlen(pad->library) == 0 || pad->package == NULL || strlen(pad->package) == 0) {
            sprintf(buffer, "Failed to get GND pad %s library/package parameters", pad->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (pad->x == 0 && pad->y == 0) {
            sprintf(buffer, "GND pad %s X and Y are 0", pad->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (pad->package_pad.shape == SHAPE_UNKNOWN) {
            sprintf(buffer, "GND pad %s missing a shape", pad->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (pad->package_pad.drill_size == 0) {
            sprintf(buffer, "GND pad %s missing a drill size", pad->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
    }
    return RESULT_OK;
}

void print_project(EagleBoardProject *project) {
    printf(""
           "design_rules:\n"
           "\tpad_hole_to_mask_ratio:\t%lf %%\n"
           "\tpad_min_mask_diameter:\t%lf mm\n"
           "\tpad_max_mask_diameter:\t%lf mm\n"
           "GND pads:\n",
           project->design_rules.pad_hole_to_mask_ratio,
           project->design_rules.pad_min_mask_diameter,
           project->design_rules.pad_max_mask_diameter
    );
    for (int i = 0; i < project->pad_count; i++) {
        printf("\tname: '%s'; gnd_pad: '%s'; library: '%s'; package: '%s'; x: %lf; y: %lf; rotation: %.0lf%s\n",
               project->pads[i].name,
               project->pads[i].package_pad.name,
               project->pads[i].library,
               project->pads[i].package,
               project->pads[i].x,
               project->pads[i].y,
               project->pads[i].rotation,
               project->pads[i].inverted ? "; inverted" : ""
        );
    }
}

int eagle_board_parse(AppState *state) {
    if (state->eagle_board == NULL || strlen(state->eagle_board->name) == 0) {
        strcpy(state->status_message, "EAGLE board cannot be empty");
        return RESULT_FAILED;
    }

    char buffer[256];
    FILE *file;
    sprintf(buffer, "%s/%s/%s.brd", state->projects_path, state->project, state->eagle_board->name);
    int result = open_file(state, buffer, &file);
    if (result != RESULT_OK) return result;

    state->eagle_board->design_rules.pad_hole_to_mask_ratio = -1;
    state->eagle_board->design_rules.pad_min_mask_diameter = -1;
    state->eagle_board->design_rules.pad_max_mask_diameter = -1;
    state->eagle_board->design_rules.pad_shape_long_ratio = 0;

    result = read_board_file(state, file, state->eagle_board);
    fclose(file);

    if (result != RESULT_OK) return result;

    result = validate_project(state, state->eagle_board);
    if (result != RESULT_OK) return result;

//    print_project(state->eagle_board);
    return RESULT_OK;
}

int read_job_file(AppState *state, FILE *file) {
    char *line = NULL;
    char *match;
    while (file_read_line(file, &line) == RESULT_OK) {
        if ((match = strstr(line, "\"ProjectId\":")) != NULL) {
            if (sscanf(match, "\"ProjectId\": \"%s\"", state->eagle_board->name) != 1) {
                state->eagle_board->name[0] = '\0';
                strcpy(state->status_message, "No board name found");
                return RESULT_EMPTY;
            }
            strip_extra_chars(state->eagle_board->name);
        } else if ((match = strstr(line, "\"X\":")) != NULL) {
            state->eagle_board->width = strtod(&(match[5]), NULL);
        } else if ((match = strstr(line, "\"Y\":")) != NULL) {
            state->eagle_board->height = strtod(&(match[5]), NULL);
        }
    }
    if (line) free(line);
    return RESULT_OK;
}

int eagle_job_parse(AppState *state) {
    char buffer[256];
    FILE *file;
    sprintf(buffer, "%s/%s/CAMOutputs/GerberFiles/gerber_job.gbrjob", state->projects_path, state->project);
    int result = open_file(state, buffer, &file);
    if (result != RESULT_OK) return result;

    if (state->eagle_board == NULL) {
        state->eagle_board = malloc(sizeof(EagleBoardProject));
    }

    result = read_job_file(state, file);
    fclose(file);

    return result;
}

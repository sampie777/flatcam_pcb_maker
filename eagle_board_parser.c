//
// Created by samuel on 22-9-22.
//

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdlib.h>
#include "eagle_board_parser.h"
#include "utils.h"
#include "return_codes.h"
#include "file_utils.h"

#define MIL_TO_MM_FACTOR 0.0254

typedef enum {
    SHAPE_UNKNOWN = 0,
    SHAPE_ROUND,
    SHAPE_OCTAGON,
    SHAPE_SQUARE,
    SHAPE_LONG
} PadShape;

typedef struct {
    char *name;
    double x;
    double y;
    double rotation;
    double drill_size;
    double diameter;
    PadShape shape;
} PackagePad;

typedef struct {
    char *name;
    char *library;
    char *package;
    PackagePad pad;
    double x;
    double y;
    double rotation;
} GndElement;

typedef struct {
    struct {
        double pad_hole_to_mask_ratio;
        double pad_min_mask_diameter;
        double pad_max_mask_diameter;
    } design_rules;
    int element_count;
    GndElement *elements;
} EagleBoardProject;

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

int read_file_designrules(AppState *state, FILE *file, EagleBoardProject *project) {
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
        }
    }

    if (line) free(line);
    return result;
}

int read_file_gnd_signal(AppState *state, FILE *file, EagleBoardProject *project) {
    project->element_count = 0;
    project->elements = NULL;

    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</signal>")) break;
        if (!starts_with(line, "<contactref ")) break;

        project->element_count++;
        project->elements = realloc(project->elements, sizeof(GndElement) * project->element_count);
        GndElement *element = &(project->elements[project->element_count - 1]);

        strtok(line, "\"");
        element->name = strtok(NULL, "\"");
        strtok(NULL, "\"");
        element->pad.name = strtok(NULL, "\"");
        element->library = NULL;
        element->package = NULL;
        element->x = 0;
        element->y = 0;
        element->rotation = 0;

        if (element->name == NULL || element->pad.name == NULL) {
            strcpy(state->status_message, "Failed to get GND element specifics");
            result = RESULT_FAILED;
            break;
        }
    }

    if (line) free(line);
    return result;
}

int read_file_elements(AppState *state, FILE *file, EagleBoardProject *project) {
    if (project->element_count == 0) return RESULT_OK;

    char *line = NULL;
    int result;
    while ((result = file_read_line(file, &line)) == RESULT_OK) {
        if (starts_with(line, "</elements>")) break;

        char name[64] = "", library[64] = "", package[64] = "", x_text[64] = "", y_text[64] = "", rotation_text[64] = "";
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


        for (int i = 0; i < project->element_count; i++) {
            GndElement *element = &(project->elements[i]);
            if (strcmp(name, element->name) != 0) continue;

            element->library = malloc(strlen(library) + 1);
            strcpy(element->library, library);
            element->package = malloc(strlen(package) + 1);
            strcpy(element->package, package);

            element->x = x;
            element->y = y;
            element->rotation = rotation;
        }
    }

    if (line) free(line);
    return result;
}

int read_file_libraries(AppState *state, FILE *file, EagleBoardProject *project) {
    if (project->element_count == 0) return RESULT_OK;

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

        if (!starts_with(line, "<gnd_pad ")) continue;

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

        for (int i = 0; i < project->element_count; i++) {
            GndElement *element = &(project->elements[i]);
            if (strcmp(library_name, element->library) != 0
                || strcmp(package_name, element->package) != 0
                || strcmp(pad_name, element->pad.name) != 0)
                continue;

            element->pad.x = x;
            element->pad.y = y;
            element->pad.rotation = rotation;
            element->pad.drill_size = drill;
            element->pad.diameter = diameter;
            element->pad.shape = shape;
        }
    }

    if (line) free(line);
    return result;
}

int read_file(AppState *state, FILE *file, EagleBoardProject *project) {
    char *line = NULL;
    int result;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "<designrules")) {
            result = read_file_designrules(state, file, project);
            if (result != RESULT_OK) break;
        } else if (starts_with(line, "<signal name=\"GND\"")) {
            result = read_file_gnd_signal(state, file, project);
            if (result != RESULT_OK) break;
        }
    }

    // Reread file from beginning
    fseek(file, 0, SEEK_SET);
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "<elements")) {
            result = read_file_elements(state, file, project);
            if (result != RESULT_OK) break;
        }
    }

    // Reread file from beginning
    fseek(file, 0, SEEK_SET);
    while (file_read_line(file, &line) == RESULT_OK) {
        if (starts_with(line, "<libraries")) {
            result = read_file_libraries(state, file, project);
            if (result != RESULT_OK) break;
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
    for (int i = 0; i < project->element_count; i++) {
        GndElement *element = &(project->elements[i]);
        if (element->library == NULL || strlen(element->library) == 0 || element->package == NULL || strlen(element->package) == 0) {
            strcpy(state->status_message, "Failed to get GND element libary/package parameters");
            sprintf(buffer, "Failed to get GND element %s libary/package parameters", element->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (element->x == 0 && element->y == 0) {
            sprintf(buffer, "GND element %s X and Y are 0", element->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (element->pad.shape == SHAPE_UNKNOWN) {
            sprintf(buffer, "GND element %s missing a shape", element->name);
            strcpy(state->status_message, buffer);
            return RESULT_FAILED;
        }
        if (element->pad.drill_size == 0) {
            sprintf(buffer, "GND element %s missing a drill size", element->name);
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
           "elements:\n",
           project->design_rules.pad_hole_to_mask_ratio,
           project->design_rules.pad_min_mask_diameter,
           project->design_rules.pad_max_mask_diameter
    );
    for (int i = 0; i < project->element_count; i++) {
        printf("\tname: '%s'; gnd_pad: '%s'; library: '%s'; package: '%s'; x: %lf; y: %lf; rotation: %.0lf\n",
               project->elements[i].name,
               project->elements[i].pad.name,
               project->elements[i].library,
               project->elements[i].package,
               project->elements[i].x,
               project->elements[i].y,
               project->elements[i].rotation
        );
    }
}

int eagle_board_parse(AppState *state, const char *file_name) {
    FILE *file;
    int result = open_file(state, file_name, &file);
    if (result != RESULT_OK) return result;

    EagleBoardProject project = {
            .design_rules.pad_hole_to_mask_ratio = -1,
            .design_rules.pad_min_mask_diameter = -1,
            .design_rules.pad_max_mask_diameter = -1,
    };

    result = read_file(state, file, &project);
    fclose(file);

    print_project(&project);

    if (result != RESULT_OK) return result;

    result = validate_project(state, &project);
    if (result != RESULT_OK) return result;

    return RESULT_OK;
}

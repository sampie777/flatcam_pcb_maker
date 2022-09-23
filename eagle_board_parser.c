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

typedef struct {
    char *name;
    char *library;
    char *package;
    char *gnd_pad;
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
        element->gnd_pad = strtok(NULL, "\"");
        element->library = NULL;
        element->package = NULL;
        element->x = 0;
        element->y = 0;
        element->rotation = 0;

        if (element->name == NULL || element->gnd_pad == NULL) {
            strcpy(state->status_message, "Failed to get GND element specifics");
            result = RESULT_FAILED;
            break;
        }
    }

    if (line) free(line);
    return result;
}

void get_gnd_element_for_name(const EagleBoardProject *project, GndElement **element, const char *name, int offset) {
    int matches = 0;
    for (int i = 0; i < project->element_count; i++) {
        if (strcmp(project->elements[i].name, name) == 0) {
            *element = &(project->elements[i]);
            matches++;

            if (matches > offset)
                break;
        }
    }
}

void merge_elements_by_library_data(AppState *state, EagleBoardProject *project) {
    for (int i = 0; i < project->element_count; i++) {
        GndElement *element = &(project->elements[i]);
        if (element->library != NULL) continue;

        GndElement *base_element = element;
        int offset = 0;
        while (base_element != NULL && base_element->library == NULL) {
            get_gnd_element_for_name(project, &base_element, element->name, offset++);
        }
        if (base_element == NULL) {
            printf("Could not find base element for %s\n", element->name);
            continue;
        }

        // Link library and package
        element->library = base_element->library;
        element->package = base_element->package;
        element->x = base_element->x;
        element->y = base_element->y;
        element->rotation = base_element->rotation;
    }
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
            sscanf(part, "rot=\"%s\"", rotation_text);
        }

        if (strlen(name) > 0) name[strlen(name) - 1] = '\0';
        if (strlen(library) > 0) library[strlen(library) - 1] = '\0';
        if (strlen(package) > 0) package[strlen(package) - 1] = '\0';
        if (strlen(x_text) > 0) x_text[strlen(x_text) - 1] = '\0';
        if (strlen(y_text) > 0) y_text[strlen(y_text) - 1] = '\0';
        if (strlen(rotation_text) > 0) rotation_text[strlen(rotation_text) - 1] = '\0';
        double x = strtod(x_text, NULL);
        double y = strtod(y_text, NULL);
        double rotation = strtod(rotation_text, NULL);

        GndElement *element = NULL;
        get_gnd_element_for_name(project, &element, name, 0);
        if (element == NULL) continue;  // Not a GND element

        element->library = malloc(strlen(library) + 1);
        strcpy(element->library, library);
        element->package = malloc(strlen(package) + 1);
        strcpy(element->package, package);

        element->x = x;
        element->y = y;
        element->rotation = rotation;
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
    if (line) free(line);

    merge_elements_by_library_data(state, project);

    if (result == RESULT_EMPTY) return RESULT_OK;
    return result;
}

int validate_project(AppState *state, EagleBoardProject *project) {
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
            return RESULT_FAILED;
        }
        if (element->x == 0 && element->y == 0) {
            strcpy(state->status_message, "GND element X and Y are 0");
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
               project->elements[i].gnd_pad,
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

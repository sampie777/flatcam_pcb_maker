//
// Created by samuel on 23-9-22.
//

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <math.h>
#include "../utils.h"
#include "gnd_pads.h"
#include "../return_codes.h"

#define GND_PAD_MAX_DISTANCE_TO_RADIUS_RATIO 1.1
#define GND_PAD_MIN_MATCH_RATIO 0.95

void calculate_location_of_pad(const AppState *state, const GndPad *pad, double *pad_x, double *pad_y) {
    double alpha = pad->rotation / 180 * M_PI;
    // Rotation matrix
    double rotated_pad_x = pad->package_pad.x * cos(alpha) - pad->package_pad.y * sin(alpha);
    double rotated_pad_y = pad->package_pad.x * sin(alpha) + pad->package_pad.y * cos(alpha);

    if (pad->inverted) {
        rotated_pad_x *= -1;
    }

    *pad_x = pad->x + rotated_pad_x;
    *pad_y = pad->y + rotated_pad_y;

    if (state->flatcam_options.mirror == 'Y') {
        *pad_x = state->eagle_board->min_x + state->eagle_board->max_x - *pad_x;
    }

    *pad_x = (state->flatcam_options.offset_x - state->eagle_board->min_x) + *pad_x;
    *pad_y = (state->flatcam_options.offset_y - state->eagle_board->min_y) + *pad_y;
}

double calculate_distance_to_pad(AppState *state, GndPad *pad, double x, double y) {
    double pad_x;
    double pad_y;
    calculate_location_of_pad(state, pad, &pad_x, &pad_y);

    return distance_between_points(pad_x, pad_y, x, y);
}

double calculate_max_pad_radius(const AppState *state, GndPad *pad) {
    double pad_diameter = pad->package_pad.diameter
                          ? pad->package_pad.diameter
                          : pad->package_pad.drill_size * (1 + state->eagle_board->design_rules.pad_hole_to_mask_ratio);

    pad_diameter += 2 * state->flatcam_options.dia_width;

    if (pad->package_pad.shape == SHAPE_LONG) {
        pad_diameter *= 1 + state->eagle_board->design_rules.pad_shape_long_ratio;
    } else if (pad->package_pad.shape == SHAPE_SQUARE) {
        // Calculate radius of circle bounding the rectangle instead of inner circling the rectangle
        pad_diameter *= sqrt(2.0);
    } else if (pad->package_pad.shape == SHAPE_OCTAGON) {
        pad_diameter *= sqrt(2.0) / cos(M_PI / 8);
    }

    return max(state->eagle_board->design_rules.pad_min_mask_diameter, pad_diameter) / 2;
}

void gnd_pads_debug(const AppState *state) {
    for (int i = 0; i < state->eagle_board->pad_count; i++) {
        GndPad *pad = &(state->eagle_board->pads[i]);

        double pad_x;
        double pad_y;
        calculate_location_of_pad(state, pad, &pad_x, &pad_y);
        double radius = calculate_max_pad_radius(state, pad);

        printf("%16s\t(%6.2lf; %6.2lf) @ %6.2lf\n", pad->name, pad_x, pad_y, radius);
    }
}

void update_removed_gnd_pads(AppState *state, GndPad *const *nearest_pads, const int *matches_per_pad, int nearest_pads_count) {
    int best_matched_pad_index = 0;
    for (int i = 0; i < nearest_pads_count; i++) {
        if (matches_per_pad[i] > matches_per_pad[best_matched_pad_index]) {
            best_matched_pad_index = i;
        }
    }

    // Find source pad for connected pad chain
    GndPad *connected_pad = nearest_pads[best_matched_pad_index];
    for (int i = 0; i < state->eagle_board->pad_count; i++) {
        if (&(state->eagle_board->pads[i]) == connected_pad) continue;
        if (state->eagle_board->pads[i].connected_to != connected_pad) continue;

        connected_pad = &(state->eagle_board->pads[i]);
        i = 0;
    }

    // Mark all (connected) pads as removed
    while (connected_pad != NULL) {
        connected_pad->has_been_removed = true;
        connected_pad = connected_pad->connected_to;
    }
}

bool is_gnd_pad_match(GndPad *const *nearest_pads, const int *matches_per_pad, int nearest_pads_count, int total_lines) {
    bool is_gnd_pad = false;
    for (int i = 0; i < nearest_pads_count; i++) {
        double match_rate = (double) matches_per_pad[i] / total_lines;

        // Include connected pads
        GndPad *connected_pad = nearest_pads[i]->connected_to;
        while (connected_pad != NULL) {
            for (int j = 0; j < nearest_pads_count; j++) {
                if (connected_pad != nearest_pads[j]) continue;
                match_rate += (double) matches_per_pad[j] / total_lines;
            }
            connected_pad = connected_pad->connected_to;
        }

        is_gnd_pad |= match_rate > GND_PAD_MIN_MATCH_RATIO;
    }
    return is_gnd_pad;
}

bool remove_gnd_pads(AppState *state, FILE *file, char **start_line) {
    if (state->eagle_board == NULL) return false;

    long start_index = ftell(file);
    GndPad *nearest_pads[16];
    int matches_per_pad[16];
    int nearest_pads_count = 0;
    int total_lines = 0;

    double x, y;
    char *line = NULL;
    while (file_read_line(file, &line) == RESULT_OK) {
        if (!starts_with(line, "G01 X")) break;

        if (sscanf(line, "G01 X%lfY%lf", &x, &y) != 2) break;

        for (int i = 0; i < state->eagle_board->pad_count; i++) {
            GndPad *pad = &(state->eagle_board->pads[i]);
            double distance_to_pad = calculate_distance_to_pad(state, pad, x, y);
            double pad_radius = calculate_max_pad_radius(state, pad);
            if (distance_to_pad > pad_radius * GND_PAD_MAX_DISTANCE_TO_RADIUS_RATIO) continue;

            bool pad_already_found = false;
            for (int j = 0; j < nearest_pads_count; j++) {
                if (nearest_pads[j] == pad) {
                    pad_already_found = true;
                    matches_per_pad[j]++;
                    break;
                }
            }
            if (pad_already_found) continue;

            nearest_pads[nearest_pads_count] = pad;
            matches_per_pad[nearest_pads_count] = 0;
            nearest_pads_count++;
        }
        total_lines++;
    }

    bool is_gnd_pad = is_gnd_pad_match(nearest_pads, matches_per_pad, nearest_pads_count, total_lines);

    if (!is_gnd_pad) {
        fseek(file, start_index, SEEK_SET);
        return false;
    }

    if (strlen(*start_line) < strlen(line)) {
        *start_line = malloc(strlen(line) + 1);
    }
    strcpy(*start_line, line);

    update_removed_gnd_pads(state, nearest_pads, matches_per_pad, nearest_pads_count);
    return true;
}

void merge_connected_gnd_pads(AppState *state) {
    if (state->eagle_board == NULL) return;

    for (int i = 0; i < state->eagle_board->pad_count; i++) {
        double pad_x;
        double pad_y;
        GndPad *pad = &(state->eagle_board->pads[i]);
        calculate_location_of_pad(state, pad, &pad_x, &pad_y);
        double radius = calculate_max_pad_radius(state, pad);

        for (int j = i - 1; j >= 0; j--) {
            GndPad *other_pad = &(state->eagle_board->pads[j]);
            double other_radius = calculate_max_pad_radius(state, other_pad);
            double distance_between_pads = calculate_distance_to_pad(state, other_pad, pad_x, pad_y);
            if (distance_between_pads >= radius + other_radius + state->flatcam_options.dia_width) continue;

            pad->connected_to = other_pad;
            break;
        }
    }
}
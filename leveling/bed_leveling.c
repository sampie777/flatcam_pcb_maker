//
// Created by samuel on 2-1-23.
//

#include <stdlib.h>
#include <memory.h>
#include "bed_leveling.h"
#include "height_calculation.h"
#include "../utils.h"

void leveling_add_measurement_row(Leveling *leveling) {
    leveling->row_length++;
    if (leveling->measurements == NULL) {
        leveling->measurements = malloc(leveling->row_length * sizeof(Point3D *));
    } else {
        leveling->measurements = realloc(leveling->measurements, leveling->row_length * sizeof(Point3D *));
    }

    leveling->measurements[leveling->row_length - 1] = malloc(leveling->column_length * sizeof(Point3D));
    memset(leveling->measurements[leveling->row_length - 1], 0, leveling->column_length * sizeof(Point3D));
}

void leveling_remove_measurement_row(Leveling *leveling) {
    if (leveling->row_length <= 0) return;

    leveling->row_length--;
    if (leveling->measurements == NULL)
        return;

    free(leveling->measurements[leveling->row_length]);
}

void leveling_add_measurement_column(Leveling *leveling) {
    leveling->column_length++;
    for (int i = 0; i < leveling->row_length; i++) {
        if (leveling->measurements[i] == NULL) {
            leveling->measurements[i] = malloc(leveling->column_length * sizeof(Point3D));
        } else {
            leveling->measurements[i] = realloc(leveling->measurements[i], leveling->column_length * sizeof(Point3D));
        }
        memset(&(leveling->measurements[i][leveling->column_length - 1]), 0, sizeof(Point3D));
    }
}

void leveling_remove_measurement_column(Leveling *leveling) {
    if (leveling->column_length <= 0) return;

    leveling->column_length--;
}

void leveling_add_measurement_point(Leveling *leveling, int column, int row, double x, double y, double z) {
    while (row >= leveling->row_length) {
        leveling_add_measurement_row(leveling);
    }
    while (column >= leveling->column_length) {
        leveling_add_measurement_column(leveling);
    }

    leveling->measurements[row][column].x = x;
    leveling->measurements[row][column].y = y;
    leveling->measurements[row][column].z = z;
}

double leveling_calculate_height_for_point(const Leveling *leveling, const Point3D *point) {
    return bilinear_interpolation(leveling, point);
}

double leveling_calculate_height_for_coordinate(const Leveling *leveling, double x, double y) {
    Point3D point = {
            .x = x,
            .y = y,
    };
    return leveling_calculate_height_for_point(leveling, &point);
}

void leveling_calculate_x_and_y_separation_for_measurement_points(AppState *state) {
    while (state->leveling.row_length) {
        leveling_remove_measurement_row(&(state->leveling));
    }
    while (state->leveling.column_length) {
        leveling_remove_measurement_column(&(state->leveling));
    }

    if (state->eagle_board == NULL) return;
    if (state->eagle_board->width == 0 || state->eagle_board->height == 0) return;

    int cols = (int) (state->eagle_board->width / state->leveling.min_distance_between_measurement_points_mm);
    double width_interval = cols > 0 ? state->eagle_board->width / cols : state->eagle_board->width;
    int rows = (int) (state->eagle_board->height / state->leveling.min_distance_between_measurement_points_mm);
    double height_interval = rows > 0 ? state->eagle_board->height / rows : state->eagle_board->height;

    for (int i = 0; i <= max(rows, 1); i++) {
        for (int j = 0; j <= max(cols, 1); j++) {
            leveling_add_measurement_point(&(state->leveling), j, i,
                                           state->flatcam_options.offset_x + j * width_interval,
                                           state->flatcam_options.offset_y + i * height_interval,
                                           0);
        }
    }
}
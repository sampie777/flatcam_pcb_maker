//
// Created by samuel on 2-1-23.
//

#include <stdlib.h>
#include <memory.h>
#include "bed_leveling.h"
#include "height_calculation.h"

void leveling_add_measurement_row(Leveling *leveling) {
    leveling->row_length++;
    if (leveling->measurements == NULL) {
        leveling->measurements = malloc(leveling->row_length * sizeof(Point3D *));
        leveling->measurements[0] = NULL;
    } else {
        leveling->measurements = realloc(leveling->measurements, leveling->row_length * sizeof(Point3D *));
        leveling->measurements[leveling->row_length - 1] = malloc(leveling->column_length * sizeof(Point3D));
        memset(leveling->measurements[leveling->row_length - 1], 0, leveling->column_length * sizeof(Point3D));
    }
}

void leveling_remove_measurement_row(Leveling *leveling) {
    if (leveling->row_length <= 0) return;

    leveling->row_length--;
    if (leveling->measurements == NULL)
        return;

    free(leveling->measurements[leveling->row_length]);
}

void leveling_add_measurement_column(Leveling *leveling) {
    if (leveling->row_length == 0) return;

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
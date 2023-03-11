//
// Created by samuel on 2-1-23.
//

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "bed_leveling.h"
#include "leveling_utils.h"

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

void calculate_center_points(Leveling *leveling) {
    leveling->center_points_length = 0;
    if (leveling->center_points != NULL) free(leveling->center_points);

    for (int row = 0; row < leveling->row_length - 1; row++) {
        for (int col = 0; col < leveling->column_length - 1; col++) {
            Point3D *point1 = &(leveling->measurements[row][col]);
            Point3D *point2 = &(leveling->measurements[row][col + 1]);
            Point3D *point3 = &(leveling->measurements[row + 1][col]);
            Point3D *point4 = &(leveling->measurements[row + 1][col + 1]);

            Point3D center_point = {
                    .x = (point1->x + point2->x + point3->x + point4->x) / 4.0,
                    .y = (point1->y + point2->y + point3->y + point4->y) / 4.0,
                    .z = (point1->z + point2->z + point3->z + point4->z) / 4.0,
            };

            leveling->center_points_length++;
            if (leveling->center_points == NULL) {
                leveling->center_points = malloc(sizeof(Point3D));
            } else {
                leveling->center_points = realloc(leveling->center_points, leveling->center_points_length * sizeof(Point3D));
            }
            memcpy(&(leveling->center_points[leveling->center_points_length - 1]), &center_point, sizeof(Point3D));
        }
    }
}

void add_measurement_point(Leveling *leveling, int column, int row, double x, double y, double z) {
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

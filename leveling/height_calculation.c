//
// Created by samuel on 11-3-23.
//

#include <stddef.h>
#include "height_calculation.h"
#include "leveling_utils.h"

double bilinear_interpolation(const Leveling *leveling, const Point3D *point) {
    Point3D *point1 = NULL, *point2 = NULL, *point3 = NULL, *point4 = NULL;

    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            add_1_point_if_closer_to_1_point(point, current_point, &point1);
        }
    }

    if (point1 == NULL) return 0;

    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            if (current_point == point1) continue;
            if (current_point->y == point1->y)
                add_1_point_if_closer_to_1_point(point, current_point, &point2);
        }
    }

    if (point2 == NULL) return 0;

    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            if (current_point == point1 || current_point == point2) continue;
            if (current_point->x == point1->x)
                add_1_point_if_closer_to_1_point(point, current_point, &point3);
        }
    }

    if (point3 == NULL) return 0;

    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            if (current_point == point1 || current_point == point2 || current_point == point3) continue;
            if (current_point->x == point2->x && current_point->y == point3->y)
                add_1_point_if_closer_to_1_point(point, current_point, &point4);
        }
    }

    if (point4 == NULL) return 0;

    return (point4->y - point->y) / (point4->y - point1->y) * ((point4->x - point->x) / (point4->x - point1->x) * point1->z + (point->x - point1->x) / (point4->x - point1->x) * point2->z) +
           (point->y - point1->y) / (point4->y - point1->y) * ((point4->x - point->x) / (point4->x - point1->x) * point3->z + (point->x - point1->x) / (point4->x - point1->x) * point4->z);

}
//
// Created by samuel on 11-3-23.
//

#include <math.h>
#include <stddef.h>
#include <malloc.h>
#include "height_calculation.h"
#include "../common.h"
#include "leveling_utils.h"
#include "../utils.h"


double leveling_calculate_height_for_point_on_2d_plane(const Point3D *plane_point1, const Point3D *plane_point2, const Point3D *point) {
    Point2D v_plane = {
            .x = plane_point2->x - plane_point1->x,
            .y = plane_point2->y - plane_point1->y,
    };
    Point2D v_point = {
            .x = point->x - plane_point1->x,
            .y = point->y - plane_point1->y,
    };

    Point2D v_projection = {
            .x = vector_dot(&v_point, &v_plane) / (vector_dot(&v_plane, &v_plane)) * v_plane.x,
            .y = vector_dot(&v_point, &v_plane) / (vector_dot(&v_plane, &v_plane)) * v_plane.y,
    };

    double angle_point = atan2(v_point.y, v_point.x);
    double angle_plane = atan2(v_plane.y, v_plane.x);
    double angle = angle_point - angle_plane;

    double distance_factor = vector_len(&v_projection) / vector_len(&v_plane);
    if (angle > 0.5 * M_PI || angle < -0.5 * M_PI) distance_factor *= -1;

    double height_difference = plane_point2->z - plane_point1->z;
    return plane_point1->z + distance_factor * height_difference;
}

double leveling_calculate_height_for_point_on_multi_plane(const Leveling *leveling, const Point3D *point) {
    Point3D *point1 = NULL, *point2 = NULL, *point3 = NULL;

    find_nearest_points_for_point(leveling, point, &point1, &point2, &point3);

    if (point3 == NULL) {
        return leveling_calculate_height_for_point_on_2d_plane(point1, point2, point);
    }

    return leveling_calculate_height_for_point_on_3d_plane(
            point1,
            point2,
            point3,
            point);
}

double leveling_calculate_height_for_point(const Leveling *leveling, const Point3D *point) {
    if (leveling->row_length == 0 || leveling->column_length == 0) return 0;
    if (leveling->row_length == 1 && leveling->column_length == 1) return leveling->measurements[0][0].z;
    if (leveling->row_length * leveling->column_length == 2) {
        return leveling_calculate_height_for_point_on_2d_plane(*(leveling->measurements), *(leveling->measurements + sizeof(Point3D)), point);
    }
    return leveling_calculate_height_for_point_on_multi_plane(leveling, point);
}

double gaussian_blur_point(const Leveling *leveling, const Point3D *point, int size, double standard_deviation) {
    static double **blur_matrix = NULL;

    if (blur_matrix == NULL) {
        blur_matrix = malloc(size * sizeof(double *));
        for (int i = 0; i < size; i++) {
            blur_matrix[i] = malloc(size * sizeof(double));
        }

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                int offset_x = j - (int) (size / 2);
                int offset_y = i - (int) (size / 2);

                double blur_coefficient = 1.0 / (2 * M_PI * standard_deviation * standard_deviation);
                blur_coefficient *= pow(M_E,
                                        -(offset_x * offset_x + offset_y * offset_y)
                                        / (2.0 * standard_deviation * standard_deviation));

                blur_matrix[i][j] = blur_coefficient;
            }
        }
    }

    double height = 0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            int offset_x = j - (int) (size / 2);
            int offset_y = i - (int) (size / 2);

            Point3D blur_point = {
                    .x = point->x + offset_x,
                    .y = point->y + offset_y,
            };

            double blur_coefficient = blur_matrix[i][j];
            height += blur_coefficient
                      * get_nearest_known_measurements_height_for_point(leveling, &blur_point); // Use known measurement points only
//                      * leveling_calculate_height_for_point(leveling, &blur_point);   // Interpolate points to get approx. height
        }
    }

    return height;
}

double gaussian_dynamic_blur_point(const Leveling *leveling, const Point3D *point) {
//    return gaussian_blur_point(leveling, point, 1, sqrt(0.5 / M_PI)); // Apply no blur
//    return gaussian_blur_point(leveling, point, 10, 2);   // Apply fixed blur
    int size = (leveling->measurements[0][1].x - leveling->measurements[0][0].x + leveling->measurements[1][0].y - leveling->measurements[0][0].y) / 1.5;
    return gaussian_blur_point(leveling, point, size + 1, 5);   // Apply blur according to measurement points
}

double gaussian_blur_5x5_point(const Leveling *leveling, const Point3D *point) {
#define size 5
    double blur_matrix[size][size] = {
            {1, 4,  6,  4,  1},
            {4, 16, 24, 16, 4},
            {6, 24, 26, 24, 6},
            {4, 16, 24, 16, 4},
            {1, 4,  6,  4,  1},
    };
    double blur_coefficient = 1.0 / 256;

    double height = 0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            Point3D blur_point = {
                    .x = point->x - (int) (size / 2) + j,
                    .y = point->y - (int) (size / 2) + i,
            };
            height += blur_coefficient * blur_matrix[i][j]
                      * leveling_calculate_height_for_point(leveling, &blur_point);
        }
    }

    return height;
}

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
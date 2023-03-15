//
// Created by samuel on 11-3-23.
//

#include "leveling_utils.h"
#include "../common.h"
#include "../utils.h"
#include "../bitmap.h"
#include "bed_leveling.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void leveling_calculate_plane(const Point3D *point1,
                              const Point3D *point2,
                              const Point3D *point3,
                              Plane3D *plane) {
    Point3D vector0 = {
            .x = point1->x - point2->x,
            .y = point1->y - point2->y,
            .z = point1->z - point2->z,
    };
    Point3D vector1 = {
            .x = point1->x - point3->x,
            .y = point1->y - point3->y,
            .z = point1->z - point3->z,
    };

    Point3D normal_vector = {
            .x = vector0.y * vector1.z - vector0.z * vector1.y,
            .y = vector0.z * vector1.x - vector0.x * vector1.z,
            .z = vector0.x * vector1.y - vector0.y * vector1.x,
    };

    plane->a = normal_vector.x;
    plane->b = normal_vector.y;
    plane->c = normal_vector.z;
    plane->d = -1 * (plane->a * point1->x + plane->b * point1->y + plane->c * point1->z);
}

double leveling_get_height_for_point_on_plane(const Plane3D *plane, const Point3D *point) {
    // Calculate z by:
    // ax + by + cz + d = 0  ->  z = -(ax + by + d) / c
    return plane->c == 0 ? 0 : -1 * (plane->a * point->x + plane->b * point->y + plane->d) / plane->c;
}

double leveling_calculate_height_for_point_on_3d_plane(const Point3D *plane_point1,
                                                       const Point3D *plane_point2,
                                                       const Point3D *plane_point3,
                                                       const Point3D *point) {
    Plane3D plane = {0};
    leveling_calculate_plane(plane_point1,
                             plane_point2,
                             plane_point3,
                             &plane);

    return leveling_get_height_for_point_on_plane(&plane, point);
}

void add_2_points_if_closer_to_1_point(const Point3D *point, Point3D *new_point,
                                       Point3D **closest_point1, Point3D **closest_point2) {
    double distance_new_point = distance_between_3d_points(point, new_point);

    if (*closest_point2 != NULL) {
        double distance_point2 = distance_between_3d_points(point, *closest_point2);
        if (distance_new_point > distance_point2 || point == *closest_point2) return;
        *closest_point2 = new_point;
    } else if (*closest_point1 != NULL) {
        *closest_point2 = new_point;
    }

    if (*closest_point1 != NULL) {
        double distance_point1 = distance_between_3d_points(point, *closest_point1);
        if (distance_new_point > distance_point1 || point == *closest_point1) return;
        *closest_point2 = *closest_point1;
        *closest_point1 = new_point;
    } else {
        *closest_point1 = new_point;
    }
}

void add_1_point_if_closer_to_1_point(const Point3D *point, Point3D *new_point,
                                      Point3D **closest_point) {
    if (*closest_point == NULL) {
        *closest_point = new_point;
    }

    double distance_new_point = distance_between_3d_points(point, new_point);
    double distance_point1 = distance_between_3d_points(point, *closest_point);
    if (distance_new_point > distance_point1 || point == *closest_point) return;
    *closest_point = new_point;
}

void add_2_points_if_closer_to_2_points(const Point3D *point1, const Point3D *point2, Point3D *new_point,
                                        Point3D **closest_point1, Point3D **closest_point2) {
    double distance_to_new_point1 = distance_between_3d_points(point1, new_point);
    double distance_to_new_point2 = distance_between_3d_points(point2, new_point);
    double total_distance_to_new_point = distance_to_new_point1 + distance_to_new_point2;

    if (*closest_point2 != NULL) {
        double distance_to_previous_point1 = distance_between_3d_points(point1, *closest_point2);
        double distance_to_previous_point2 = distance_between_3d_points(point2, *closest_point2);
        double total_distance_to_previous_point = distance_to_previous_point1 + distance_to_previous_point2;

        if (total_distance_to_new_point > total_distance_to_previous_point
            || point1 == *closest_point2 || point2 == *closest_point2) {
            return;
        }
        *closest_point2 = new_point;
    } else {
        *closest_point2 = new_point;
    }


    if (*closest_point1 != NULL) {
        double distance_to_previous_point1 = distance_between_3d_points(point1, *closest_point1);
        double distance_to_previous_point2 = distance_between_3d_points(point2, *closest_point1);
        double total_distance_to_previous_point = distance_to_previous_point1 + distance_to_previous_point2;

        if (total_distance_to_new_point > total_distance_to_previous_point
            || point1 == *closest_point1 || point2 == *closest_point1)
            return;
        *closest_point2 = *closest_point1;
        *closest_point1 = new_point;
    } else {
        *closest_point1 = new_point;
    }
}

void find_point_closest_to_3(const Point3D *point1, const Point3D *point2, const Point3D *point3,
                             Point3D *new_point,
                             Point3D **closest_point) {
    if (new_point == NULL) return;
    if (*closest_point == NULL) {
        *closest_point = new_point;
        return;
    }

    double distance_to_new_point1 = distance_between_3d_points(point1, new_point);
    double distance_to_new_point2 = distance_between_3d_points(point2, new_point);
    double distance_to_new_point3 = distance_between_3d_points(point3, new_point);
    double total_distance_to_new_point = distance_to_new_point1 + distance_to_new_point2 + distance_to_new_point3;

    double distance_to_previous_point1 = distance_between_3d_points(point1, *closest_point);
    double distance_to_previous_point2 = distance_between_3d_points(point2, *closest_point);
    double distance_to_previous_point3 = distance_between_3d_points(point3, *closest_point);
    double total_distance_to_previous_point = distance_to_previous_point1 + distance_to_previous_point2 + distance_to_previous_point3;

    if (total_distance_to_new_point > total_distance_to_previous_point
        || point1 == *closest_point || point2 == *closest_point || point3 == *closest_point)
        return;
    *closest_point = new_point;
}

void find_nearest_points_for_point(const Leveling *leveling, const Point3D *point,
                                   Point3D **point1, Point3D **point2, Point3D **point3) {
    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            add_2_points_if_closer_to_1_point(point, current_point, point1, point2);
        }
    }

    Point3D *center_point1 = NULL;
    Point3D *center_point2 = NULL;
    for (int i = 0; i < leveling->center_points_length; i++) {
        Point3D *current_point = &(leveling->center_points[i]);
//        add_1_point_if_closer_to_1_point(point, current_point, point3);
        // Next doesn't seem to do anything different
        // first get one|two points closest to `point1` and `point2`.
        add_2_points_if_closer_to_2_points(*point1, *point2, current_point, &center_point1, &center_point2);
//        find_point_closest_to_3(point, *point1, *point2, current_point, point3);
    }
    find_point_closest_to_3(point, *point1, *point2, center_point1, point3);
    find_point_closest_to_3(point, *point1, *point2, center_point2, point3);
    // Then take the point closest to `point` of these one|two points.
//    if (center_point1 == NULL) return;
//    add_1_point_if_closer_to_1_point(point, center_point1, point3);
//    if (center_point2 == NULL) return;
//    add_1_point_if_closer_to_1_point(point, center_point2, point3);
}

double get_nearest_known_measurements_height_for_point(const Leveling *leveling, const Point3D *point) {
    Point3D *measurement_point = NULL;
    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *current_point = &(leveling->measurements[row][col]);
            add_1_point_if_closer_to_1_point(point, current_point, &measurement_point);
        }
    }
    for (int i = 0; i < leveling->center_points_length; i++) {
//        Point3D *current_point = &(leveling->center_points[i]);
//        add_1_point_if_closer_to_1_point(point, current_point, &measurement_point);
    }
    return measurement_point->z;
}

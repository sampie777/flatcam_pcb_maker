//
// Created by samuel on 10-4-23.
//

#include <memory.h>
#include <stdlib.h>
#include "gcode_modifier_line_mapper.h"


Array2DIndex *get_cluster_for_point(const Leveling *leveling, double x, double y) {
    Array2DIndex *cluster = malloc(sizeof(Array2DIndex));
    cluster->column = 0;
    cluster->row = 0;
    for (int row = 0; row < leveling->row_length; row++) {
        for (int col = 0; col < leveling->column_length; col++) {
            Point3D *point = &(leveling->measurements[row][col]);

            if (point->x < x) cluster->column = col;
            if (point->y < y) cluster->row = row;
            if (point->x > x && point->y > y) break;
        }
    }
    return cluster;
}

int add_intersection_points_for_horizontal_cluster(
        const Leveling *leveling,
        double from_x, double from_y,
        double to_x, double to_y,
        const Array2DIndex *from_cluster,
        const Array2DIndex *to_cluster,
        Point2D **added_points,
        int start_index) {
    if (to_x == from_x) return 0;

    // Create line formula (y=ax+b) which we can use to calculate the exact point on each cluster intersection
    double a = (to_y - from_y) / (to_x - from_x);
    double b = from_y - a * from_x;


    int added_points_size = 0;
    for (int col = from_cluster->column;
         col != to_cluster->column + (from_cluster->column < to_cluster->column ? 1 : 0);
         col += (from_cluster->column < to_cluster->column ? 1 : -1)) {
        Point3D *measurement_point = &(leveling->measurements[0][col]);
        if (!(measurement_point->x > from_x && measurement_point->x < to_x)
            && !(measurement_point->x < from_x && measurement_point->x > to_x))
            continue;

        (*added_points)[start_index + added_points_size].x = measurement_point->x;
        (*added_points)[start_index + added_points_size].y = a * measurement_point->x + b;
        added_points_size++;
    }
    return added_points_size;
}

int add_intersection_points_for_vertical_cluster(
        const Leveling *leveling,
        double from_x, double from_y,
        double to_x, double to_y,
        const Array2DIndex *from_cluster,
        const Array2DIndex *to_cluster,
        Point2D **added_points,
        int start_index) {
    // Create line formula (y=ax+b) which we can use to calculate the exact point on each cluster intersection
    double a = (to_y - from_y) / (to_x - from_x);
    double b = from_y - a * from_x;

    int added_points_size = 0;
    for (int row = from_cluster->row;
         row != to_cluster->row + (from_cluster->row < to_cluster->row ? 1 : 0);
         row += (from_cluster->row < to_cluster->row ? 1 : -1)) {
        Point3D *measurement_point = &(leveling->measurements[row][0]);
        if (!(measurement_point->y > from_y && measurement_point->y < to_y)
            && !(measurement_point->y < from_y && measurement_point->y > to_y))
            continue;

        // If `x` is the same, `a` is infinitive (division by zero), so no formula can be applied
        (*added_points)[start_index + added_points_size].x = (to_x == from_x) ? from_x : (measurement_point->y - b) / a;
        (*added_points)[start_index + added_points_size].y = measurement_point->y;
        added_points_size++;
    }
    return added_points_size;
}

void point2d_array_reverse(Point2D **points, size_t length) {
    for (int i = 0; i < length / 2; i++) {
        Point2D *point = &((*points)[i]);
        Point2D *next_point = &((*points)[length - 1 - i]);

        Point2D temp_point = {0};
        memcpy(&temp_point, point, sizeof(Point2D));
        memcpy(point, next_point, sizeof(Point2D));
        memcpy(next_point, &temp_point, sizeof(Point2D));
    }
}

void point2d_array_sort(Point2D **points, size_t length, bool reverse) {
    if (length < 2) return;

    for (int i = 0; i < length; i++) {
        Point2D *point = &((*points)[i]);
        for (int j = i + 1; j < length; j++) {
            Point2D *next_point = &((*points)[j]);
            if (point->x <= next_point->x) continue;

            Point2D temp_point = {0};
            memcpy(&temp_point, point, sizeof(Point2D));
            memcpy(point, next_point, sizeof(Point2D));
            memcpy(next_point, &temp_point, sizeof(Point2D));
        }
    }

    if (!reverse) return;
    point2d_array_reverse(points, length);
}

size_t get_intersection_points_on_line(Leveling *leveling, double from_x, double from_y, double to_x, double to_y, Point2D **added_points) {
    if (leveling->row_length == 0 || leveling->column_length == 0) return 0;

    Array2DIndex *from_cluster = get_cluster_for_point(leveling, from_x, from_y);
    Array2DIndex *to_cluster = get_cluster_for_point(leveling, to_x, to_y);

    if (from_cluster->column == to_cluster->column && from_cluster->row == to_cluster->row) {
        free(from_cluster);
        free(to_cluster);
        return 0;
    }

    int cluster_column_difference = to_cluster->column - from_cluster->column;
    int cluster_row_difference = abs(to_cluster->row - from_cluster->row);
    *added_points = malloc((abs(cluster_column_difference) + abs(cluster_row_difference)) * sizeof(Point2D));
    int added_points_size = 0;  // Will be used to keep track of added items to the added_points array

    added_points_size += add_intersection_points_for_horizontal_cluster(leveling, from_x, from_y, to_x, to_y, from_cluster, to_cluster, added_points, added_points_size);
    added_points_size += add_intersection_points_for_vertical_cluster(leveling, from_x, from_y, to_x, to_y, from_cluster, to_cluster, added_points, added_points_size);

    free(from_cluster);
    free(to_cluster);

    point2d_array_sort(added_points, added_points_size, from_x > to_x);
    return added_points_size;
}
//
// Created by samuel on 10-4-23.
//

#ifndef FLATCAM_PCB_MAKER_GCODE_MODIFIER_LINE_MAPPER_H
#define FLATCAM_PCB_MAKER_GCODE_MODIFIER_LINE_MAPPER_H

#include <stddef.h>
#include "../common.h"

/**
 * For this function to work, we expect that leveling row=0 starts with the smallest Y and leveling col=0 the smallest X.
 * A cluster is defined as the rectangular area between four measurement points,
 * identified by the row-column index of the smallest X/Y measurement point
 * @param leveling
 * @param x
 * @param y
 * @return
 */
Array2DIndex *get_cluster_for_point(const Leveling *leveling, double x, double y);

/**
 * Calculate the exact intersection point of the line through the different measurement clusters.
 * @param leveling
 * @param from_x
 * @param from_y
 * @param to_x
 * @param to_y
 * @param from_cluster
 * @param to_cluster
 * @param added_points
 * @param start_index
 * @return
 */
int add_intersection_points_for_horizontal_cluster(
        const Leveling *leveling,
        double from_x, double from_y,
        double to_x, double to_y,
        const Array2DIndex *from_cluster,
        const Array2DIndex *to_cluster,
        Point2D **added_points,
        int start_index);

int add_intersection_points_for_vertical_cluster(
        const Leveling *leveling,
        double from_x, double from_y,
        double to_x, double to_y,
        const Array2DIndex *from_cluster,
        const Array2DIndex *to_cluster,
        Point2D **added_points,
        int start_index);

void point2d_array_reverse(Point2D **points, size_t length);

void point2d_array_sort(Point2D **points, size_t length, bool reverse);

/**
 * Creates a sorted array of points of each cluster intersection that occurs on the line between 'from' and 'to'.
 * @param leveling
 * @param from_x
 * @param from_y
 * @param to_x
 * @param to_y
 * @param added_points
 * @return size of 'added_points', 0 if empty.
 */
size_t get_intersection_points_on_line(Leveling *leveling, double from_x, double from_y, double to_x, double to_y, Point2D **added_points);

#endif //FLATCAM_PCB_MAKER_GCODE_MODIFIER_LINE_MAPPER_H

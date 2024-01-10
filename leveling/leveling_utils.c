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

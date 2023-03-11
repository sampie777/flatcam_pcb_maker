//
// Created by samuel on 11-3-23.
//

#ifndef FLATCAM_PCB_MAKER_LEVELING_UTILS_H
#define FLATCAM_PCB_MAKER_LEVELING_UTILS_H

#include "../common.h"

void add_1_point_if_closer_to_1_point(const Point3D *point, Point3D *new_point, Point3D **closest_point);

void find_nearest_points_for_point(const Leveling *leveling, const Point3D *point,
                                   Point3D **point1, Point3D **point2, Point3D **point3);

double leveling_calculate_height_for_point_on_3d_plane(const Point3D *plane_point1,
                                                       const Point3D *plane_point2,
                                                       const Point3D *plane_point3,
                                                       const Point3D *point);

double get_nearest_known_measurements_height_for_point(const Leveling *leveling, const Point3D *point);

#endif //FLATCAM_PCB_MAKER_LEVELING_UTILS_H

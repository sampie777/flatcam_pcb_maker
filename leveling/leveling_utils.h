//
// Created by samuel on 11-3-23.
//

#ifndef FLATCAM_PCB_MAKER_LEVELING_UTILS_H
#define FLATCAM_PCB_MAKER_LEVELING_UTILS_H

#include "../common.h"

void add_1_point_if_closer_to_1_point(const Point3D *point, Point3D *new_point, Point3D **closest_point);

#endif //FLATCAM_PCB_MAKER_LEVELING_UTILS_H

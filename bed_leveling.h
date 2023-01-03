//
// Created by samuel on 2-1-23.
//

#ifndef FLATCAM_PCB_MAKER_BED_LEVELING_H
#define FLATCAM_PCB_MAKER_BED_LEVELING_H

#include "common.h"

void bed_leveling_calculate(AppState *state);

void bed_leveling_calculate_printer_leveling_points(const Plane3D *plane,
                                                    int mesh_size,
                                                    double mesh_x_min,
                                                    double mesh_x_max,
                                                    double mesh_y_min,
                                                    double mesh_y_max,
                                                    Point3D ***level_points);

#endif //FLATCAM_PCB_MAKER_BED_LEVELING_H

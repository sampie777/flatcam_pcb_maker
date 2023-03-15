//
// Created by samuel on 2-1-23.
//

#ifndef FLATCAM_PCB_MAKER_BED_LEVELING_H
#define FLATCAM_PCB_MAKER_BED_LEVELING_H

#include "../common.h"

void leveling_add_measurement_row(Leveling *leveling);

void leveling_remove_measurement_row(Leveling *leveling);

void leveling_add_measurement_column(Leveling *leveling);

void leveling_remove_measurement_column(Leveling *leveling);

void leveling_add_measurement_point(Leveling *leveling, int column, int row, double x, double y, double z);

double leveling_calculate_height_for_point(const Leveling *leveling, const Point3D *point);

void leveling_calculate_x_and_y_separation_for_measurement_points(AppState *state);

#endif //FLATCAM_PCB_MAKER_BED_LEVELING_H

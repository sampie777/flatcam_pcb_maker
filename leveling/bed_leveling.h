//
// Created by samuel on 2-1-23.
//

#ifndef FLATCAM_PCB_MAKER_BED_LEVELING_H
#define FLATCAM_PCB_MAKER_BED_LEVELING_H

#include "../common.h"
// todo: remove
void leveling_add_measurement_point(Leveling *leveling);

// todo: remove
void leveling_remove_measurement_point(Leveling *leveling, int index);

// todo: remove
double leveling_calculate_height_for_point(const Leveling *leveling, const Point3D *point);


void leveling_add_measurement_row(Leveling *leveling);

void leveling_remove_measurement_row(Leveling *leveling);

void leveling_add_measurement_column(Leveling *leveling);

void add_measurement_point(Leveling *leveling, int column, int row, double x, double y, double z);

#endif //FLATCAM_PCB_MAKER_BED_LEVELING_H

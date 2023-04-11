//
// Created by samuel on 11-3-23.
//

#ifndef FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H
#define FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H

#include "../common.h"

double bilinear_interpolation(const Leveling *leveling, const Point3D *point);

#endif //FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H

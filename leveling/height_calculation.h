//
// Created by samuel on 11-3-23.
//

#ifndef FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H
#define FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H

#include "../common.h"

double gaussian_dynamic_blur_point(const Leveling *leveling, const Point3D *point);

double gaussian_blur_5x5_point(const Leveling *leveling, const Point3D *point);

double bilinear_interpolation(const Leveling *leveling, const Point3D *point);

#endif //FLATCAM_PCB_MAKER_HEIGHT_CALCULATION_H

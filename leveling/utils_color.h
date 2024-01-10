//
// Created by samuel on 4-5-23.
//

#ifndef FLATCAM_PCB_MAKER_UTILS_COLOR_H
#define FLATCAM_PCB_MAKER_UTILS_COLOR_H

typedef struct rgb {
    char r, g, b;
} RGB;

typedef struct hsl {
    double h, s, l;
} HSL;

HSL rgb2hsl(char red, char green, char blue);

double hue2rgb(double p, double q, double t);

RGB hsl2rgb(double h, double s, double l);

#endif //FLATCAM_PCB_MAKER_UTILS_COLOR_H

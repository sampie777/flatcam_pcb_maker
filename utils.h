//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_UTILS_H
#define FLATCAM_PCB_MAKER_UTILS_H

#include <stdbool.h>
#include "common.h"

#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

int bound_int(int value, int min, int max, bool roll_over);

double bound_double(double value, double min, double max);

void copy_to_clipboard(const char *data);

void open_folder(const char *path);

bool starts_with(const char *source, const char *needle);

void string_replace(char *input, char needle, char replacement);

void auto_format_double_string(char *input);

double distance_between_points(double x1, double y1, double x2, double y2);

double distance_between_3d_points(const Point3D *a, const Point3D *b);

double vector_dot(const Point2D *a, const Point2D *b);

double vector_len(const Point2D *a);

void wait_seconds(int seconds);

#endif //FLATCAM_PCB_MAKER_UTILS_H

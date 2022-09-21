//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_UTILS_H
#define FLATCAM_PCB_MAKER_UTILS_H

#include <stdbool.h>

#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

int bound_int(int value, int min, int max, bool roll_over);

double bound_double(double value, double min, double max);

void copy_to_clipboard(const char *data);

bool starts_with(const char *source, const char *needle);

void string_replace(char *input, char needle, char replacement);

#endif //FLATCAM_PCB_MAKER_UTILS_H

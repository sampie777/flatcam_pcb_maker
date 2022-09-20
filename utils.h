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

int bound(int value, int min, int max, bool roll_over);

void copy_to_clipboard(const char *data);

bool starts_with(const char *source, const char *needle);

#endif //FLATCAM_PCB_MAKER_UTILS_H

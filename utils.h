//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_UTILS_H
#define FLATCAM_PCB_MAKER_UTILS_H

#include <stdbool.h>

int bound(int value, int min, int max, bool roll_over);

void copy_to_clipboard(const char *data);

#endif //FLATCAM_PCB_MAKER_UTILS_H

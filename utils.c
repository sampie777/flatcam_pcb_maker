//
// Created by samuel on 16-9-22.
//

#include "utils.h"

int bound(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
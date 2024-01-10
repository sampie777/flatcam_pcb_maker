//
// Created by samuel on 9-3-23.
//

#ifndef KILO_TUTORIAL_BITMAP_H
#define KILO_TUTORIAL_BITMAP_H

#include <stdbool.h>

typedef struct {
    int width;
    int height;
    int scale;
    char ***data;
} BitMap;

void bitmap_malloc(BitMap *bitmap);

void bitmap_save(const BitMap *bitmap, const char* filename);

#endif //KILO_TUTORIAL_BITMAP_H

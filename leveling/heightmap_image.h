//
// Created by samuel on 15-3-23.
//

#ifndef FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H
#define FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H

#include "../common.h"

void leveling_create_bitmap(Leveling *leveling, const char *filename);

void leveling_create_terminal_image(Leveling *leveling, int width, char **output);

#endif //FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H

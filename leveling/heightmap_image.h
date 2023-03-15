//
// Created by samuel on 15-3-23.
//

#ifndef FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H
#define FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H

#include "../common.h"

void create_bitmap(Leveling *leveling);

void create_terminal_image(Leveling *leveling, int width, char **output);

#endif //FLATCAM_PCB_MAKER_HEIGHTMAP_IMAGE_H

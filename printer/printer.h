//
// Created by samuel on 1-5-23.
//

#ifndef FLATCAM_PCB_MAKER_PRINTER_H
#define FLATCAM_PCB_MAKER_PRINTER_H

#include "../common.h"
#include "serial.h"

double printer_find_height_for(SerialDevice *device, double x, double y);

void printer_init(SerialDevice *device);

void printer_disconnect(SerialDevice *device);

void printer_finish(SerialDevice *device);

#endif //FLATCAM_PCB_MAKER_PRINTER_H

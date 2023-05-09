//
// Created by samuel on 1-5-23.
//

#ifndef FLATCAM_PCB_MAKER_PRINTER_H
#define FLATCAM_PCB_MAKER_PRINTER_H

#include "../common.h"
#include "serial.h"

void printer_move_to(AppState *state, SerialDevice *device, double x, double y, bool (*should_stop)(AppState *state, SerialDevice *device));

double printer_find_height_for(AppState *state, SerialDevice *device, double x, double y, bool (*should_stop)(AppState *state, SerialDevice *device));

void printer_init(AppState *state, SerialDevice *device, bool (*should_stop)(AppState *state, SerialDevice *device));

void printer_disconnect(SerialDevice *device);

void printer_finish(SerialDevice *device);

#endif //FLATCAM_PCB_MAKER_PRINTER_H

//
// Created by samuel on 1-5-23.
//

#ifndef FLATCAM_PCB_MAKER_SERIAL_H
#define FLATCAM_PCB_MAKER_SERIAL_H

#include <stdbool.h>

typedef struct {
    int port;
    bool opened;
    char *port_name;
    bool has_error;
    char error[256];
} SerialDevice;

void serial_open(SerialDevice *device);

void serial_close(SerialDevice *device);

void serial_read_line(SerialDevice *device, char *buffer, size_t max_length);

void serial_read(SerialDevice *device);

void serial_read_clear(const SerialDevice *device, int iterations);

void serial_println(SerialDevice *device, const char *data);

#endif //FLATCAM_PCB_MAKER_SERIAL_H

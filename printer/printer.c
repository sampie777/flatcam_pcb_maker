//
// Created by samuel on 1-5-23.
//

#include <stdio.h>
#include <string.h>
#include "printer.h"
#include "serial.h"
#include "../utils.h"

#define G_LINEAR_MOVE_NON_EXTRUSION "G00"
#define G_LINEAR_MOVE_EXTRUSION "G01"
#define G_PAUSE_MILLISECONDS "G04 P"
#define G_PAUSE_SECONDS "G04 S"
#define G_MILLIMETER_UNITS "G21"
#define G_AUTO_HOME "G28"
#define G_ABSOLUTE_POSITIONING "G90"
#define G_SET_SPINDLE_CW "M03"
#define G_SPINDLE_OFF "M05"
#define G_ENABLE_STEPPERS "M17"
#define G_DISABLE_STEPPERS "M18"
#define G_PAUSE_SD_PRINT "M25"
#define G_GET_CURRENT_POSITION "M114"
#define G_GET_ENDSTOP_STATES "M119"
#define G_PLAY_TONE "M300 S2000 P500"

/*
 * When asking the printer for endstop states, the following response can be expected:
```Reporting endstop status
x_min: open
y_min: open
z_min: TRIGGERED
filament: TRIGGERED
ok P15 B3
```
 */
bool printer_endstop_z_triggered(AppState *state, SerialDevice *device, bool (*should_stop)(AppState *state, SerialDevice *device)) {
    char buffer[256];
    serial_read_clear(device, 2);
    serial_println(device, G_GET_ENDSTOP_STATES);
    serial_read_line(device, buffer, sizeof(buffer));

    // find triggered part
    while (!starts_with(buffer, "Reporting endstop status")) {
        if (should_stop(state, device)) return false;

        serial_read_line(device, buffer, sizeof(buffer));
    }

    char *part = strtok(buffer, "\n");
    while ((part = strtok(NULL, "\n")) != NULL) {
        if (!starts_with(part, "z_min: ")) continue;

        char *part2 = strtok(part, " ");
        while ((part2 = strtok(NULL, " ")) != NULL) {
            if (strcmp(part2, "open") == 0) {
                return false;
            }
        }
        return true;
    }
    return true;
}

void printer_move_to(AppState *state, SerialDevice *device, double x, double y, bool (*should_stop)(AppState *state, SerialDevice *device)) {
    if (!device->opened || should_stop(state, device)) return;
    char buffer[256];

    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" Z5.00");
    sprintf(buffer, G_LINEAR_MOVE_NON_EXTRUSION" X%.4lfY%.4lf", x, y);
    serial_println(device, buffer);

    serial_read_clear(device, 4);
    wait_seconds(1);
}

double printer_find_height_for(AppState *state, SerialDevice *device, double x, double y, bool (*should_stop)(AppState *state, SerialDevice *device)) {
    if (!device->opened || should_stop(state, device)) return 2;
    char buffer[256];

    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" Z5.00");
    sprintf(buffer, G_LINEAR_MOVE_NON_EXTRUSION" X%.4lfY%.4lf", x, y);
    serial_println(device, buffer);

    wait_seconds(1);
    serial_read_clear(device, 6);

    double z = 2;
    while (!printer_endstop_z_triggered(state, device, should_stop) && z > 0) {
        if (!device->opened || should_stop(state, device)) return z;

        z -= 0.01;
        sprintf(buffer, G_LINEAR_MOVE_EXTRUSION" Z%.4lf", z);
        serial_println(device, buffer);
    }
    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" Z5.00");

    return z;
}

void printer_auto_home(AppState *state, SerialDevice *device, bool (*should_stop)(AppState *state, SerialDevice *device)) {
    serial_println(device, G_AUTO_HOME);

    char buffer[256];
    while (device->opened && !should_stop(state, device)) {
        serial_read_line(device, buffer, sizeof(buffer));
        if (starts_with(buffer, "X:0.00 Y:0.00")) {
            // Homing done
            return;
        }
    }
}

void printer_init(AppState *state, SerialDevice *device, bool (*should_stop)(AppState *state, SerialDevice *device)) {
    serial_open(device);
    if (!device->opened || should_stop(state, device)) return;

    serial_read_clear(device, 10);
    serial_println(device, G_MILLIMETER_UNITS);
    serial_println(device, G_ABSOLUTE_POSITIONING);
    serial_println(device, G_ENABLE_STEPPERS);
    printer_auto_home(state, device, should_stop);

    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" F3000");
    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" Z5.00");
    serial_println(device, G_PAUSE_MILLISECONDS"1");

    serial_read_clear(device, 10);
}

void printer_disconnect(SerialDevice *device) {
    serial_close(device);
}

void printer_finish(SerialDevice *device) {
    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" Z30.00");
    serial_println(device, G_LINEAR_MOVE_NON_EXTRUSION" X0.00");
    serial_println(device, G_DISABLE_STEPPERS);
    serial_println(device, G_PLAY_TONE);
    wait_seconds(2);
}

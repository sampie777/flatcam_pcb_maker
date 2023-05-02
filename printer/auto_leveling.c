//
// Created by samuel on 2-5-23.
//

#include <stdlib.h>
#include <string.h>
#include "auto_leveling.h"
#include "../terminal_utils.h"
#include "../screen.h"
#include "serial.h"
#include "printer.h"

#define SERIAL_PORT "/dev/ttyUSB0"


void process_key_press(AppState *state) {
    int c = editorReadKey(false);
    if (c == -1) return;

    switch (c) {
        case '\r':  // ENTER key
            if (state->leveling.auto_leveling_status == AUTO_LEVELING_STATUS_IDLE) {
                state->leveling.auto_leveling_status = AUTO_LEVELING_STATUS_SHOULD_START;
            } else {
                state->leveling.auto_leveling_status = AUTO_LEVELING_STATUS_SHOULD_STOP;
            }
            break;
        case CTRL_KEY('q'):
            clearScreen();
            exit(0);
    }
}

bool should_stop(Leveling *leveling, const SerialDevice *device) {
    leveling->device_has_error = device->has_error;
    strcpy(leveling->device_error, device->error);
    return leveling->auto_leveling_status == AUTO_LEVELING_STATUS_SHOULD_STOP || device->has_error;
}

void auto_leveling_loop(AppState *state, SerialDevice *device) {
    screen_refresh(state);
    printer_init(device);

    for (int i = 0; i < state->leveling.row_length; i++) {
        for (int j = 0; j < state->leveling.column_length; j++) {
            process_key_press(state);
            if (should_stop(&(state->leveling), device)) return;

            Point3D *point = &(state->leveling.measurements[i][j]);
            point->z = printer_find_height_for(device, point->x, point->y);

            screen_refresh(state);
        }
    }

    process_key_press(state);
    if (should_stop(&(state->leveling), device)) return;

    printer_finish(device);
}

void auto_leveling_run(AppState *state) {
    state->screen = SCREEN_PRINTER_LEVELING;
    state->printer_leveling_selection = PRINTER_LEVELING_BUTTON_AUTO_LEVEL;
    state->leveling.auto_leveling_status = AUTO_LEVELING_STATUS_RUNNING;

    SerialDevice device = {
            .port_name = SERIAL_PORT,
    };

    auto_leveling_loop(state, &device);

    printer_disconnect(&device);
    state->leveling.auto_leveling_status = AUTO_LEVELING_STATUS_IDLE;
}

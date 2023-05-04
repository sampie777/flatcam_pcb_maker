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

void update_device_error(AppState *state, const SerialDevice *device) {
    if (state->leveling.auto_leveling_status == AUTO_LEVELING_STATUS_SHOULD_STOP) {
        state->leveling.device_has_error = false;
        strcpy(state->leveling.device_error, "Interrupted");
    } else {
        state->leveling.device_has_error = device->has_error;
        strcpy(state->leveling.device_error, device->error);
    }
}

bool should_stop(AppState *state, SerialDevice *device) {
    process_key_press(state);
    update_device_error(state, device);
    screen_refresh(state);
    return state->leveling.auto_leveling_status == AUTO_LEVELING_STATUS_SHOULD_STOP || device->has_error;
}

void auto_leveling_loop(AppState *state, SerialDevice *device) {
    screen_refresh(state);
    printer_init(state, device, should_stop);

    for (int i = 0; i < state->leveling.row_length; i++) {
        for (int j = 0; j < state->leveling.column_length; j++) {
            if (should_stop(state, device)) return;

            Point3D *point = &(state->leveling.measurements[i][j]);
            point->z = printer_find_height_for(state, device, point->x, point->y, should_stop);
        }
    }

    if (should_stop(state, device)) return;

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

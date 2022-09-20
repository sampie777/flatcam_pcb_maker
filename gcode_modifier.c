//
// Created by samuel on 20-9-22.
//

#include <string.h>
#include "gcode_modifier.h"

void gcode_modify(AppState *state) {
    strcpy(state->status_message, "Modified!");
}

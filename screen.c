//
// Created by samuel on 16-9-22.
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "screen.h"

#define NEW_LINE "\r\n\x1b[K"

void bufferAppend(ScreenBuffer *screen_buffer, const char *s) {
    int len = strlen(s);
    // Get new memory block which is big enough for our data
    char *new = realloc(screen_buffer->buffer, screen_buffer->length + len);
    if (new == NULL)
        return;

    // Append the new string to our buffer
    memcpy(&new[screen_buffer->length], s, len);
    // Update buffer properties
    screen_buffer->buffer = new;
    screen_buffer->length += len;
}

void bufferFree(ScreenBuffer *screen_buffer) {
    free(screen_buffer->buffer);
}

void enable_highlight(ScreenBuffer *screen_buffer, int value) {
    if (value) {
        bufferAppend(screen_buffer, "\x1b[7m"); // Invert colors
    } else {
        bufferAppend(screen_buffer, "\x1b[m"); // Re-Invert colors
    }
}

void draw_option(ScreenBuffer *screen_buffer, int action_index, const char *title, int highlight) {
    if (highlight) {
        enable_highlight(screen_buffer, 1);
    }

    char buffer[256];
    sprintf(buffer, "% 4d    %s ", action_index, title);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);

    if (highlight) {
        enable_highlight(screen_buffer, 0);
    }
}

void draw_button(ScreenBuffer *screen_buffer, const char *title, int highlight) {
    if (highlight) {
        enable_highlight(screen_buffer, 1);
    }

    char buffer[256];
    sprintf(buffer, "[ %s ]", title);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);

    if (highlight) {
        enable_highlight(screen_buffer, 0);
    }
}

void draw_text_field(ScreenBuffer *screen_buffer, const char *title, const char *value, int highlight) {
    char buffer[256];
    sprintf(buffer, "   %-20s  ", title);
    bufferAppend(screen_buffer, buffer);

    if (highlight) {
        enable_highlight(screen_buffer, 1);
    }

    sprintf(buffer, " %s ", value);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);

    if (highlight) {
        enable_highlight(screen_buffer, 0);
    }
}

void draw_select_project_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "SELECT PROJECT");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    char buffer[256];
    for (int i = 0; i < state->projects_count; i++) {
        if (state->project_selection == i) {
            enable_highlight(screen_buffer, 1);
        }

        sprintf(buffer, "% 4d    %s ", i, state->projects[i]);
        bufferAppend(screen_buffer, buffer);
        bufferAppend(screen_buffer, NEW_LINE);

        if (state->project_selection == i) {
            enable_highlight(screen_buffer, 0);
        }
    }
}

void draw_select_action_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, state->project);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    draw_option(screen_buffer, ACTION_GENERATE_FLATCAM_CODE, "Generate FlatCAM code", state->action_selection == ACTION_GENERATE_FLATCAM_CODE);
    draw_option(screen_buffer, ACTION_MODIFY_GCODE, "Modify Gcode", state->action_selection == ACTION_MODIFY_GCODE);
    draw_option(screen_buffer, ACTION_SHOW_CHECKLIST, "Checklist", state->action_selection == ACTION_SHOW_CHECKLIST);
}

void draw_generate_flatcam_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "GENERATE FLATCAM");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    bufferAppend(screen_buffer, "Options");
    bufferAppend(screen_buffer, NEW_LINE);
    draw_text_field(screen_buffer, "Copper layer", state->flatcam_options.traces, state->flatcam_option_selection == FLATCAM_COPPER_LAYER);
    draw_text_field(screen_buffer, "Offset X", state->flatcam_options.offset_x, state->flatcam_option_selection == FLATCAM_OFFSET_X);
    draw_text_field(screen_buffer, "Offset Y", state->flatcam_options.offset_y, state->flatcam_option_selection == FLATCAM_OFFSET_Y);
    draw_text_field(screen_buffer, "Dia width", state->flatcam_options.dia_width, state->flatcam_option_selection == FLATCAM_DIA_WIDTH);
    draw_text_field(screen_buffer, "Feedrate", state->flatcam_options.feedrate_etch, state->flatcam_option_selection == FLATCAM_FEEDRATE);
    draw_text_field(screen_buffer, "Silkscreen top", state->flatcam_options.silkscreen_top, state->flatcam_option_selection == FLATCAM_SILKSCREEN_TOP);
    draw_text_field(screen_buffer, "Silkscreen bottom", state->flatcam_options.silkscreen_bottom, state->flatcam_option_selection == FLATCAM_SILKSCREEN_BOTTOM);

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Generate", state->flatcam_option_selection == FLATCAM_BUTTON_GENERATE);
    draw_button(screen_buffer, "Back", state->flatcam_option_selection == FLATCAM_BUTTON_BACK);
}

void draw_show_checklist_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "CHECKLIST");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    draw_button(screen_buffer, "Back", 1);
}

void draw_dialog(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[256];
    bufferAppend(screen_buffer, NEW_LINE);
    sprintf(buffer, "%s (default: %s)", state->dialog.title, state->dialog.default_value);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    bufferAppend(screen_buffer, "  ");
    enable_highlight(screen_buffer, 1);
    char buffer2[256];
    sprintf(buffer2, "%s%s", state->dialog.value, strlen(state->dialog.value) < state->dialog.max_length ? "_" : "");
    sprintf(buffer, " %-*s ", state->dialog.max_length, buffer2);
    bufferAppend(screen_buffer, buffer);
    enable_highlight(screen_buffer, 0);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "Press <Enter> to confirm");
    bufferAppend(screen_buffer, NEW_LINE);
}

void screen_draw(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, "\x1b[K"); // Clear line

    if (state->dialog.show) {
        draw_dialog(state, screen_buffer);
        return;
    }

    switch (state->screen) {
        case SCREEN_SELECT_PROJECT:
            draw_select_project_screen(state, screen_buffer);
            break;
        case SCREEN_SELECT_ACTION:
            draw_select_action_screen(state, screen_buffer);
            break;
        case SCREEN_GENERATE_FLATCAM:
            draw_generate_flatcam_screen(state, screen_buffer);
            break;
        case SCREEN_SHOW_CHECKLIST:
            draw_show_checklist_screen(state, screen_buffer);
            break;
    }
}

void clear_remaining(AppState *state, ScreenBuffer *screen_buffer) {
    // Count new lines
    int line_count = 0;
    for (int i = 0; i < screen_buffer->length; i++) {
        if (screen_buffer->buffer[i] == '\n') {
            line_count++;
        }
    }

    for (int y = line_count + 1; y < state->row_count; y++) {
        bufferAppend(screen_buffer, NEW_LINE);
    }
}

void screen_refresh(AppState *state) {
    ScreenBuffer screen_buffer = SCREEN_BUFFER_INIT;

    bufferAppend(&screen_buffer, "\x1b[?25l");  // Hide cursor
    bufferAppend(&screen_buffer, "\x1b[H");     // Move cursor to 1,1

    screen_draw(state, &screen_buffer);
    clear_remaining(state, &screen_buffer);

    bufferAppend(&screen_buffer, "\x1b[?25h");  // Show cursor
    write(STDOUT_FILENO, screen_buffer.buffer, screen_buffer.length);
    bufferFree(&screen_buffer);
}
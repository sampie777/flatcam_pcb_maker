//
// Created by samuel on 16-9-22.
//

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include "screen.h"
#include "utils.h"
#include "checklist.h"
#include "app_version.h"
#include "leveling/heightmap_image.h"

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

void draw_horizontal_centered(AppState *state, ScreenBuffer *screen_buffer, const char *text) {
    size_t length = strlen(text);
    for (int i = 0; i < (state->column_count - length) / 2; i++) {
        bufferAppend(screen_buffer, " ");
    }
    bufferAppend(screen_buffer, text);
    for (int i = (int) ((state->column_count - length) / 2 + length); i < state->column_count; i++) {
        bufferAppend(screen_buffer, " ");
    }
}

void draw_option(ScreenBuffer *screen_buffer, int action_index, const char *title, bool highlight) {
    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    char buffer[256];
    sprintf(buffer, "% 4d    %s ", action_index, title);
    bufferAppend(screen_buffer, buffer);
    if (highlight) {
        enable_highlight(screen_buffer, false);
    }
    bufferAppend(screen_buffer, NEW_LINE);

}

void draw_button(ScreenBuffer *screen_buffer, const char *title, bool highlight) {
    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    char buffer[256];
    sprintf(buffer, "[ %s ]", title);
    bufferAppend(screen_buffer, buffer);

    if (highlight) {
        enable_highlight(screen_buffer, false);
    }
}

void draw_text_field_string(ScreenBuffer *screen_buffer, const char *title, const char *value, bool highlight) {
    char buffer[256];
    sprintf(buffer, "   %-20s  ", title);
    bufferAppend(screen_buffer, buffer);

    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    sprintf(buffer, " %s ", value);
    bufferAppend(screen_buffer, buffer);
    if (highlight) {
        enable_highlight(screen_buffer, false);
    }
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_text_field_char(ScreenBuffer *screen_buffer, const char *title, char value, bool highlight) {
    char buffer[256];
    sprintf(buffer, "   %-20s  ", title);
    bufferAppend(screen_buffer, buffer);

    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    sprintf(buffer, " %c ", value);
    bufferAppend(screen_buffer, buffer);
    if (highlight) {
        enable_highlight(screen_buffer, false);
    }
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_text_field_bool(ScreenBuffer *screen_buffer, const char *title, bool value, bool highlight) {
    char buffer[256];
    sprintf(buffer, "   %-20s  ", title);
    bufferAppend(screen_buffer, buffer);

    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    sprintf(buffer, " %c ", value ? 'Y' : 'N');
    bufferAppend(screen_buffer, buffer);
    if (highlight) {
        enable_highlight(screen_buffer, false);
    }
    bufferAppend(screen_buffer, NEW_LINE);
}

/**
 * Set title to NULL to generate an inline field.
 * Specify title as a string to create a full line field.
 * @param screen_buffer
 * @param title
 * @param value
 * @param format
 * @param highlight
 */
void draw_text_field_double(ScreenBuffer *screen_buffer, const char *title, double value, const char *format, bool highlight) {
    char buffer[256];
    if (title != NULL) {
        sprintf(buffer, "   %-20s  ", title);
        bufferAppend(screen_buffer, buffer);
    }

    if (highlight) {
        enable_highlight(screen_buffer, true);
    }

    sprintf(buffer, format, value);
    bufferAppend(screen_buffer, " ");
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, " ");
    if (highlight) {
        enable_highlight(screen_buffer, false);
    }

    if (title != NULL) {
        bufferAppend(screen_buffer, NEW_LINE);
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
            enable_highlight(screen_buffer, true);
        }

        sprintf(buffer, "% 4d    %s ", i, state->projects[i]);
        bufferAppend(screen_buffer, buffer);

        if (state->project_selection == i) {
            enable_highlight(screen_buffer, false);
        }

        bufferAppend(screen_buffer, NEW_LINE);
    }

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Quit", state->project_selection == state->projects_count);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_select_action_screen(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[128];
    bufferAppend(screen_buffer, NEW_LINE);
    sprintf(buffer, "PROJECT: %s%s%s", SCREEN_COLOR_CYAN, state->project, SCREEN_COLOR_RESET);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);

    if (state->eagle_board != NULL) {
        sprintf(buffer, "Board dimensions: [%.1lf, %.1lf]", state->eagle_board->width, state->eagle_board->height);
        bufferAppend(screen_buffer, buffer);
        bufferAppend(screen_buffer, NEW_LINE);
    }
    bufferAppend(screen_buffer, NEW_LINE);

    draw_option(screen_buffer, ACTION_GENERATE_FLATCAM_COMMANDS, "Generate FlatCAM commands", state->action_selection == ACTION_GENERATE_FLATCAM_COMMANDS);
    if (state->eagle_board != NULL)
        draw_option(screen_buffer, ACTION_PRINTER_LEVELING, "Printer leveling", state->action_selection == ACTION_PRINTER_LEVELING);
    draw_option(screen_buffer, ACTION_MODIFY_GCODE, "Modify Gcode", state->action_selection == ACTION_MODIFY_GCODE);
    draw_option(screen_buffer, ACTION_SHOW_CHECKLIST, "Checklist", state->action_selection == ACTION_SHOW_CHECKLIST);

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Back", state->action_selection == ACTION_BUTTON_BACK);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_generate_flatcam_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "GENERATE FLATCAM");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    char buffer[64];
    bufferAppend(screen_buffer, "Traces & drills");
    bufferAppend(screen_buffer, NEW_LINE);
    draw_text_field_char(screen_buffer, "Copper layer", state->flatcam_options.traces, state->flatcam_option_selection == FLATCAM_COPPER_LAYER);
    draw_text_field_char(screen_buffer, "Mirror", state->flatcam_options.mirror, state->flatcam_option_selection == FLATCAM_MIRROR);
    draw_text_field_char(screen_buffer, "Cutout profile", state->flatcam_options.cutout_profile, state->flatcam_option_selection == FLATCAM_CUTOUT_PROFILE);

    sprintf(buffer, "%lf", state->flatcam_options.offset_x);
    auto_format_double_string(buffer);
    draw_text_field_string(screen_buffer, "Offset X", buffer, state->flatcam_option_selection == FLATCAM_OFFSET_X);

    sprintf(buffer, "%lf", state->flatcam_options.offset_y);
    auto_format_double_string(buffer);
    draw_text_field_string(screen_buffer, "Offset Y", buffer, state->flatcam_option_selection == FLATCAM_OFFSET_Y);

    sprintf(buffer, "%lf", state->flatcam_options.dia_width);
    auto_format_double_string(buffer);
    draw_text_field_string(screen_buffer, "Dia width", buffer, state->flatcam_option_selection == FLATCAM_DIA_WIDTH);

    draw_text_field_string(screen_buffer, "Feedrate", state->flatcam_options.feedrate_etch, state->flatcam_option_selection == FLATCAM_FEEDRATE);

    sprintf(buffer, "%d", state->flatcam_options.iterations);
    auto_format_double_string(buffer);
    draw_text_field_string(screen_buffer, "Iterations", buffer, state->flatcam_option_selection == FLATCAM_ITERATIONS);

    draw_text_field_char(screen_buffer, "Remove GND pads", state->flatcam_options.remove_gnd_pads, state->flatcam_option_selection == FLATCAM_REMOVE_GND_PADS);
    draw_text_field_bool(screen_buffer, "Use printer bed mesh", state->printer.use_bed_leveling_mesh, state->flatcam_option_selection == FLATCAM_USE_PRINTER_BED_MESH);

    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "Silkscreen");
    bufferAppend(screen_buffer, NEW_LINE);
    draw_text_field_char(screen_buffer, "Silkscreen top", state->flatcam_options.silkscreen_top, state->flatcam_option_selection == FLATCAM_SILKSCREEN_TOP);
    draw_text_field_char(screen_buffer, "Silkscreen bottom", state->flatcam_options.silkscreen_bottom, state->flatcam_option_selection == FLATCAM_SILKSCREEN_BOTTOM);
    if (state->flatcam_options.silkscreen_top == 'Y' || state->flatcam_options.silkscreen_bottom == 'Y') {
        draw_text_field_char(screen_buffer, "Mirror silkscreen", state->flatcam_options.silkscreen_mirror, state->flatcam_option_selection == FLATCAM_SILKSCREEN_MIRROR);
    }

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Generate", state->flatcam_option_selection == FLATCAM_BUTTON_GENERATE);
    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Back", state->flatcam_option_selection == FLATCAM_BUTTON_BACK);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_modify_gcode_screen(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "GCODE MODIFIED");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    for (int i = 0; i < state->modify_results.message_count; i++) {
        bufferAppend(screen_buffer, state->modify_results.messages[i]);
        bufferAppend(screen_buffer, NEW_LINE);
    }

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Show files", state->modify_gcode_selection == MODIFY_GCODE_OPEN_FILES);
    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Back", state->modify_gcode_selection == MODIFY_GCODE_BUTTON_BACK);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_show_checklist_screen(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[256];
    bufferAppend(screen_buffer, NEW_LINE);
    sprintf(buffer, "CHECKLIST  [%d/%d]", state->checklist_check_position, checklist_length);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    int max_items = state->row_count - 10;
    int start_index = max(0, state->checklist_check_position - max_items);
    for (int i = start_index; i <= state->checklist_check_position + 1 && i < checklist_length; i++) {
        bool highlight = i == state->checklist_check_position && state->checklist_selection == CHECKLIST_NEXT_CHECK;

        if (highlight) enable_highlight(screen_buffer, true);

        if (checklist_checks[i][0] == '-') {
            sprintf(buffer, "      %s ", checklist_checks[i]);
        } else {
            sprintf(buffer, "%s [%c]  %s %s", i < state->checklist_check_position ? SCREEN_COLOR_GREEN : "", i < state->checklist_check_position ? 'X' : ' ', checklist_checks[i], SCREEN_COLOR_RESET);
        }
        bufferAppend(screen_buffer, buffer);

        if (highlight) enable_highlight(screen_buffer, false);
        bufferAppend(screen_buffer, NEW_LINE);
    }

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Back", state->checklist_selection == CHECKLIST_BUTTON_BACK);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_show_printer_leveling_screen(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[256];
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "PCB LEVELING");
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    bufferAppend(screen_buffer, "Columns:  ");
    sprintf(buffer, "  %2d   ", state->leveling.column_length);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);

    bufferAppend(screen_buffer, "Rows:     ");
    sprintf(buffer, "  %2d   ", state->leveling.row_length);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    if (state->leveling.column_length > 0 && state->leveling.row_length > 0) {
        bufferAppend(screen_buffer, SCREEN_COLOR_YELLOW" Y\\X "SCREEN_COLOR_RESET);
        Point3D *current_point;
        for (int j = 0; j < state->leveling.column_length; j++) {
            current_point = &(state->leveling.measurements[0][j]);
            sprintf(buffer, SCREEN_COLOR_YELLOW" %5.1lf"SCREEN_COLOR_RESET, current_point->x);
            bufferAppend(screen_buffer, buffer);
        }
        bufferAppend(screen_buffer, NEW_LINE);

        for (int i = state->leveling.row_length - 1; i >= 0; i--) {
            current_point = &(state->leveling.measurements[i][0]);
            sprintf(buffer, SCREEN_COLOR_YELLOW"%5.1lf "SCREEN_COLOR_RESET, current_point->y);
            bufferAppend(screen_buffer, buffer);

            for (int j = 0; j < state->leveling.column_length; j++) {
                current_point = &(state->leveling.measurements[i][j]);
                draw_text_field_double(screen_buffer, NULL,
                                       current_point->z, "%4.2lf",
                                       state->printer_leveling_selection == PRINTER_LEVELING_SELECTION_Z
                                       && state->printer_leveling_measurement_selected_index == (state->leveling.row_length - i - 1) * state->leveling.column_length + j);
            }
            bufferAppend(screen_buffer, NEW_LINE);
        }
    }

    bufferAppend(screen_buffer, NEW_LINE);
    draw_button(screen_buffer, "Back", state->printer_leveling_selection == PRINTER_LEVELING_BUTTON_BACK);
    bufferAppend(screen_buffer, " ");
    draw_button(screen_buffer, "Save image", state->printer_leveling_selection == PRINTER_LEVELING_BUTTON_SAVE_IMAGE);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    if (state->leveling.column_length == 0 || state->leveling.row_length == 0) return;

    char *terminal_image = NULL;
    leveling_create_terminal_image(&(state->leveling), min(50, state->column_count - 3), &terminal_image);

    if (terminal_image == NULL) return;
    bufferAppend(screen_buffer, terminal_image);
    free(terminal_image);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_dialog(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[256];
    bufferAppend(screen_buffer, NEW_LINE);
    sprintf(buffer, "%s (current: %s)", state->dialog.title, state->dialog.default_value);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);

    if (strlen(state->dialog.char_options) == 0) {
        bufferAppend(screen_buffer, "  ");
        enable_highlight(screen_buffer, true);
        char buffer2[250];
        sprintf(buffer2, "%s%s", state->dialog.value, strlen(state->dialog.value) < state->dialog.max_length ? "_" : "");
        sprintf(buffer, " %-*s ", state->dialog.max_length, buffer2);
        bufferAppend(screen_buffer, buffer);
        enable_highlight(screen_buffer, false);
    } else {
        for (int i = 0; i < strlen(state->dialog.char_options); i++) {
            sprintf(buffer, "%c", state->dialog.char_options[i]);
            draw_option(screen_buffer, i, buffer, state->dialog_selection == i);
        }
    }

    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, NEW_LINE);
    bufferAppend(screen_buffer, "Press <Enter> to confirm");
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_app_title(AppState *state, ScreenBuffer *screen_buffer) {
    char buffer[64];
    sprintf(buffer, "PCB MAKER  v%s", APP_VERSION);

    int left_padding_count = (int) (state->column_count - strlen(buffer)) / 2;
    char left_padding[left_padding_count + 1];
    for (int i = 0; i < left_padding_count; i++) {
        left_padding[i] = ' ';
    }
    left_padding[left_padding_count] = '\0';

    bufferAppend(screen_buffer, left_padding);
    bufferAppend(screen_buffer, SCREEN_COLOR_CYAN);
    bufferAppend(screen_buffer, buffer);
    bufferAppend(screen_buffer, SCREEN_COLOR_RESET);
    bufferAppend(screen_buffer, NEW_LINE);
}

void draw_status_message(AppState *state, ScreenBuffer *screen_buffer) {
    size_t message_length = strlen(state->status_message);
    if (message_length == 0) {
        draw_app_title(state, screen_buffer);
        return;
    }

    enable_highlight(screen_buffer, true);

    char buffer[state->column_count];
    memset(buffer, '\0', state->column_count);
    int column = 0;
    for (int i = 0; i < message_length; i++) {
        buffer[column++] = state->status_message[i];
        if (column >= state->column_count - 4 || state->status_message[i] == '\n') {
            draw_horizontal_centered(state, screen_buffer, buffer);
            bufferAppend(screen_buffer, NEW_LINE);
            memset(buffer, '\0', state->column_count);
            column = 0;
        }
    }
    draw_horizontal_centered(state, screen_buffer, buffer);

    enable_highlight(screen_buffer, false);
    bufferAppend(screen_buffer, NEW_LINE);
}

void screen_draw(AppState *state, ScreenBuffer *screen_buffer) {
    bufferAppend(screen_buffer, "\x1b[K"); // Clear line

    draw_status_message(state, screen_buffer);

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
        case SCREEN_MODIFY_GCODE:
            draw_modify_gcode_screen(state, screen_buffer);
            break;
        case SCREEN_SHOW_CHECKLIST:
            draw_show_checklist_screen(state, screen_buffer);
            break;
        case SCREEN_PRINTER_LEVELING:
            draw_show_printer_leveling_screen(state, screen_buffer);
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

    for (int y = line_count + 1; y < state->row_count - 1; y++) {
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
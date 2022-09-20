//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_COMMON_H
#define FLATCAM_PCB_MAKER_COMMON_H

#include <termios.h>

#define PROJECTS_PATH "/home/samuel"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DELETE_KEY,
};

enum Screens {
    SCREEN_SELECT_PROJECT = 0,
    SCREEN_SELECT_ACTION,
    SCREEN_GENERATE_FLATCAM,
    SCREEN_SHOW_CHECKLIST,
};

enum ProjectActions {
    ACTION_GENERATE_FLATCAM_COMMANDS = 0,
    ACTION_MODIFY_GCODE,
    ACTION_SHOW_CHECKLIST,
    ACTION_BUTTON_BACK,
    ACTION_MAX_VALUE
};

enum FlatcamOptions {
    FLATCAM_COPPER_LAYER = 0,
    FLATCAM_MIRROR,
    FLATCAM_OFFSET_X,
    FLATCAM_OFFSET_Y,
    FLATCAM_DIA_WIDTH,
    FLATCAM_FEEDRATE,
    FLATCAM_SILKSCREEN_TOP,
    FLATCAM_SILKSCREEN_BOTTOM,
    FLATCAM_SILKSCREEN_MIRROR,
    FLATCAM_BUTTON_GENERATE,
    FLATCAM_BUTTON_BACK,
    FLATCAM_MAX_VALUE
};

typedef struct {
    char traces;
    char mirror;
    char offset_x[8];
    char offset_y[8];
    char dia_width[10];
    char feedrate_etch[8];
    char silkscreen_top;
    char silkscreen_bottom;
    char silkscreen_mirror;
} FlatcamOptions;

typedef struct {
    int show;
    char title[64];
    char default_value[64];
    char value[64];
    char *destination;
    int max_length;
    void (*callback)(void*);
    char type;
    char char_options[32];
} DialogOptions;

typedef struct {
    int row_count;
    int column_count;
    enum Screens screen;
    char **projects;
    char *project;
    int projects_count;

    int project_selection;
    int action_selection;
    int flatcam_option_selection;
    int dialog_selection;
    FlatcamOptions flatcam_options;
    DialogOptions dialog;
    char status_message[256];
} AppState;

typedef struct {
    char *buffer;
    int length;
} ScreenBuffer;

#endif //FLATCAM_PCB_MAKER_COMMON_H

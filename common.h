//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_COMMON_H
#define FLATCAM_PCB_MAKER_COMMON_H

#include <stdbool.h>
#include "local_settings.h"

#define TRACES_OUTPUT_FILE "0_draw_traces.gcode"
#define PRE_DRILLS_OUTPUT_FILE "1_pre_drill_holes.gcode"
#define DRILLS_CHECK_OUTPUT_FILE "2_check_holes.gcode"
#define DRILLS_OUTPUT_FILE "3_drill_holes.gcode"
#define DRILLS_MIRRORED_CHECK_OUTPUT_FILE "4_check_mirrored_holes_cnc.gcode"
#define SILKSCREEN_OUTPUT_FILE "5_draw_silkscreen.gcode"
#define SOLDER_MASK_OUTPUT_FILE "6_cut_solder_mask.gcode"

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
    SCREEN_MODIFY_GCODE,
    SCREEN_SHOW_CHECKLIST,
    SCREEN_PRINTER_LEVELING,
};

enum ProjectActions {
    ACTION_GENERATE_FLATCAM_COMMANDS = 0,
    ACTION_PRINTER_LEVELING,
    ACTION_MODIFY_GCODE,
    ACTION_SHOW_CHECKLIST,
    ACTION_BUTTON_BACK,
    ACTION_MAX_VALUE
};

enum FlatcamOptions {
    FLATCAM_COPPER_LAYER = 0,
    FLATCAM_MIRROR,
    FLATCAM_CUTOUT_PROFILE,
    FLATCAM_OFFSET_X,
    FLATCAM_OFFSET_Y,
    FLATCAM_DIA_WIDTH,
    FLATCAM_FEEDRATE,
    FLATCAM_ITERATIONS,
    FLATCAM_REMOVE_GND_PADS,
    FLATCAM_USE_PRINTER_BED_MESH,
    FLATCAM_SILKSCREEN_TOP,
    FLATCAM_SILKSCREEN_BOTTOM,
    FLATCAM_SILKSCREEN_MIRROR,
    FLATCAM_DRILL_OFFSET_X,
    FLATCAM_DRILL_OFFSET_Y,
    FLATCAM_BUTTON_GENERATE,
    FLATCAM_BUTTON_BACK,
    FLATCAM_MAX_VALUE
};

enum ModifyGcodeActions {
    MODIFY_GCODE_BUTTON_BACK = 0,
    MODIFY_GCODE_OPEN_FILES,
    MODIFY_GCODE_MAX_VALUE,
};

enum ChecklistActions {
    CHECKLIST_NEXT_CHECK = 0,
    CHECKLIST_BUTTON_BACK,
    CHECKLIST_MAX_VALUE
};

enum PrinterLevelingActions {
    PRINTER_LEVELING_SELECTION_Z,
    PRINTER_LEVELING_BUTTON_AUTO_LEVEL,
    PRINTER_LEVELING_BUTTON_BACK,
    PRINTER_LEVELING_BUTTON_SAVE_IMAGE,
    PRINTER_LEVELING_MAX_VALUE,
};

typedef struct {
    char traces;
    char mirror;
    char cutout_profile;
    double offset_x;
    double offset_y;
    double dia_width;
    char feedrate_etch[8];
    int iterations;
    char remove_gnd_pads;
    char silkscreen_top;
    char silkscreen_bottom;
    char silkscreen_mirror;
    double drill_offset_x;
    double drill_offset_y;
} FlatcamOptions;

typedef struct {
    int show;
    char title[64];
    char default_value[64];
    char value[64];
    char *destination_char;
    double *destination_double;
    int *destination_int;
    int max_length;

    void (*callback)(void *);

    char type;
    char char_options[32];
} DialogOptions;

typedef enum {
    SHAPE_UNKNOWN = 0,
    SHAPE_ROUND,
    SHAPE_OCTAGON,
    SHAPE_SQUARE,
    SHAPE_LONG
} PadShape;

typedef struct {
    char *name;
    double x;
    double y;
    double rotation;
    double drill_size;
    double diameter;
    PadShape shape;
} PackagePad;

struct GndPadStruct {
    char *name;
    char *library;
    char *package;
    PackagePad package_pad;
    double x;
    double y;
    double rotation;
    bool inverted;
    bool has_been_removed;
    struct GndPadStruct *connected_to;
};
typedef struct GndPadStruct GndPad;

typedef struct {
    char name[128];
    double width;
    double height;
    double min_x;
    double min_y;
    double max_x;
    double max_y;
    struct {
        double pad_hole_to_mask_ratio;
        double pad_min_mask_diameter;
        double pad_max_mask_diameter;
        double pad_shape_long_ratio;
    } design_rules;
    int pad_count;
    GndPad *pads;
} EagleBoardProject;

typedef struct {
    int message_count;
    char **messages;
} ModifyGcodeResults;

typedef struct {
    double x;
    double y;
} Point2D;

typedef struct {
    int row;
    int column;
} Array2DIndex;

typedef struct {
    double x;
    double y;
    double z;
} Point3D;

typedef struct {
    double a;
    double b;
    double c;
    double d;
} Plane3D;

typedef struct {
    bool use_bed_leveling_mesh;
} PrinterSettings;

typedef enum {
    AUTO_LEVELING_STATUS_IDLE = 0,
    AUTO_LEVELING_STATUS_SHOULD_START,
    AUTO_LEVELING_STATUS_RUNNING,
    AUTO_LEVELING_STATUS_SHOULD_STOP,
} AutoLevelingStatus;

typedef struct {
    AutoLevelingStatus auto_leveling_status;
    double min_distance_between_measurement_points_mm;
    int column_length;
    int row_length;
    Point3D **measurements;
    bool device_has_error;
    char device_error[256];
} Leveling;

typedef struct {
    int row_count;
    int column_count;
    enum Screens screen;
    char *projects_path;
    char **projects;
    char *project;
    int projects_count;

    int project_selection;
    enum ProjectActions action_selection;
    enum FlatcamOptions flatcam_option_selection;
    enum ModifyGcodeActions modify_gcode_selection;
    int dialog_selection;
    enum ChecklistActions checklist_selection;
    int checklist_check_position;
    enum PrinterLevelingActions printer_leveling_selection;
    int printer_leveling_measurement_selected_index;

    FlatcamOptions flatcam_options;
    DialogOptions dialog;
    char status_message[256];
    EagleBoardProject *eagle_board;
    ModifyGcodeResults modify_results;
    PrinterSettings printer;
    Leveling leveling;
} AppState;

typedef struct {
    char *buffer;
    int length;
} ScreenBuffer;

#endif //FLATCAM_PCB_MAKER_COMMON_H

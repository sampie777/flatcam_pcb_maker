//
// Created by samuel on 19-9-22.
//

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include "flatcam_generator.h"
#include "utils.h"

void generate_silkscreen_commands(const AppState *state, char **output) {
    int should_mirror = state->flatcam_options.silkscreen_mirror == 'Y';
    int should_silkscreen_top = state->flatcam_options.silkscreen_top == 'Y';
    int should_silkscreen_bottom = state->flatcam_options.silkscreen_bottom == 'Y';
    int should_silkscreen = should_silkscreen_top || should_silkscreen_bottom;

    *output = NULL;
    if (!should_silkscreen) return;

    char traces_top_output[512];
    traces_top_output[0] = '\0';
    if (should_silkscreen_top) {
        sprintf(traces_top_output, "open_gerber \"%s/%s/CAMOutputs/GerberFiles/silkscreen_top.gbr\" -follow 1 -outname silkscreen_top\n"
                                   "follow silkscreen_top -outname silkscreen_top.follow\n"
                                   "offset silkscreen_top.follow %lf %lf\n"
                                   "%s",
                state->projects_path, state->project,
                state->flatcam_options.offset_x - state->eagle_board->min_x,
                state->flatcam_options.offset_y - state->eagle_board->min_y,
                should_mirror ? "mirror silkscreen.follow -axis Y -box profile\n" : "");
    }

    char traces_bottom_output[512];
    traces_bottom_output[0] = '\0';
    if (should_silkscreen_bottom) {
        sprintf(traces_bottom_output, "open_gerber \"%s/%s/CAMOutputs/GerberFiles/silkscreen_bottom.gbr\" -follow 1 -outname silkscreen_bottom\n"
                                      "follow silkscreen_bottom -outname silkscreen_bottom.follow\n"
                                      "offset silkscreen_bottom.follow %lf %lf\n"
                                      "%s",
                state->projects_path, state->project,
                state->flatcam_options.offset_x - state->eagle_board->min_x,
                state->flatcam_options.offset_y - state->eagle_board->min_y,
                should_mirror ? "mirror silkscreen.follow -axis Y -box profile\n" : "");
    }

    *output = malloc(strlen(traces_top_output) + strlen(traces_bottom_output) + 512);
    sprintf(*output, "\n\n%s%s"
                     "join_geometries silkscreen_joined %s %s\n"
                     "cncjob silkscreen_joined -z_cut 0.0 -z_move 2.0 -feedrate %s -tooldia 0.2032\n"
                     "write_gcode silkscreen_joined_cnc \"%s/%s/CAMOutputs/flatCAM/%s\"",
            traces_top_output, traces_bottom_output,
            should_silkscreen_top ? "silkscreen_top.follow" : "",
            should_silkscreen_bottom ? "silkscreen_bottom.follow" : "",
            state->flatcam_options.feedrate_etch,
            state->projects_path, state->project, SILKSCREEN_OUTPUT_FILE
    );
}

void generate_script(const AppState *state) {
    int should_mirror = state->flatcam_options.mirror == 'Y';
    int traces_are_on_bottom = state->flatcam_options.traces == 'B';

    char *traces_file = traces_are_on_bottom ? "copper_bottom" : "copper_top";

    char *silkscreen_output;
    generate_silkscreen_commands(state, &silkscreen_output);

    char output[2048];
    sprintf(output, "open_gerber \"%s/%s/CAMOutputs/GerberFiles/profile.gbr\" -outname profile\n"
                    "offset profile %lf %lf\n"
                    "%s"
                    "cutout profile -dia 0.1 -margin -0.2 -gapsize 0.0 -gaps tb\n"
                    "\n"
                    "open_gerber \"%s/%s/CAMOutputs/GerberFiles/%s.gbr\" -outname traces\n"
                    "offset traces %lf %lf\n"
                    "%s"
                    "isolate traces -dia %lf -passes %d -overlap 1 -combine 1 -outname traces.iso\n"
                    "\n"
                    "join_geometries joined profile_cutout traces.iso\n"
                    "cncjob joined -z_cut 0.0 -z_move 2.0 -feedrate %s -tooldia 0.2032\n"
                    "write_gcode joined_cnc \"%s/%s/CAMOutputs/flatCAM/%s\"\n"
                    "\n"
                    "open_excellon \"%s/%s/CAMOutputs/DrillFiles/drill_1_16.xln\" -outname drills\n"
                    "offset drills %lf %lf\n"
                    "%s"
                    "drillcncjob drills -drillz 0.3 -travelz 2.5 -feedrate 1000.0 -tools 1 -outname check_holes_cnc\n"
                    "drillcncjob drills -drillz -3.0 -travelz 1.5 -feedrate 1000.0 -tools 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 -outname drill_holes_cnc\n"
                    "write_gcode check_holes_cnc \"%s/%s/CAMOutputs/flatCAM/%s\"\n"
                    "write_gcode drill_holes_cnc \"%s/%s/CAMOutputs/flatCAM/%s\""
                    "%s"
                    "%s",
            state->projects_path, state->project,
            state->flatcam_options.offset_x - state->eagle_board->min_x,
            state->flatcam_options.offset_y - state->eagle_board->min_y,
            should_mirror ? "mirror profile -axis Y -box profile\n" : "",

            state->projects_path, state->project, traces_file,
            state->flatcam_options.offset_x - state->eagle_board->min_x,
            state->flatcam_options.offset_y - state->eagle_board->min_y,
            should_mirror ? "mirror traces -axis Y -box profile\n" : "",
            state->flatcam_options.dia_width, state->flatcam_options.iterations,

            state->flatcam_options.feedrate_etch,
            state->projects_path, state->project, TRACES_OUTPUT_FILE,

            state->projects_path, state->project,
            state->flatcam_options.offset_x - state->eagle_board->min_x,
            state->flatcam_options.offset_y - state->eagle_board->min_y,
            should_mirror ? "mirror drills -axis Y -box profile\n" : "",

            state->projects_path, state->project, DRILLS_CHECK_OUTPUT_FILE,
            state->projects_path, state->project, DRILLS_OUTPUT_FILE,

            silkscreen_output ? silkscreen_output : "",
            should_mirror ? "" : "\nplot"
    );

    free(silkscreen_output);

    copy_to_clipboard(output);

    // Check if output folder exists
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM", state->projects_path, state->project);
    struct stat st;
    if (stat(buffer, &st) != 0) {
        // Creating output directory
        mkdir(buffer, 0700);
    }
}

void flatcam_generate(AppState *state) {
    generate_script(state);
    strcpy(state->status_message, "Copied!");
}

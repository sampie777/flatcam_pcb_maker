//
// Created by samuel on 19-9-22.
//

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "flatcam_generator.h"
#include "utils.h"

void generate_script(const AppState *state) {
    int should_mirror = strcmp(state->flatcam_options.mirror, "y") == 0 || strcmp(state->flatcam_options.mirror, "Y") == 0;
    int traces_are_on_bottom = strcmp(state->flatcam_options.traces, "b") == 0 || strcmp(state->flatcam_options.traces, "B") == 0;
    int should_silkscreen_top = strcmp(state->flatcam_options.silkscreen_top, "T") == 0 || strcmp(state->flatcam_options.silkscreen_top, "t") == 0;
    int should_silkscreen_bottom = strcmp(state->flatcam_options.silkscreen_bottom, "B") == 0 || strcmp(state->flatcam_options.silkscreen_bottom, "b") == 0;
    int should_silkscreen = should_silkscreen_top && should_silkscreen_bottom;

    char *traces_file = traces_are_on_bottom ? "copper_bottom" : "copper_top";

    char silkscreen_output[512];
    silkscreen_output[0] = '\0';
    if (should_silkscreen) {
        sprintf(silkscreen_output, "open_gerber \"%s/%s/CAMOutputs/GerberFiles/silkscreen_%s.gbr\" -follow 1 -outname silkscreen\n"
                                   "follow silkscreen -outname silkscreen.follow\n"
                                   "offset silkscreen.follow %s %s\n"
                                   "join_geometries joined silkscreen.follow joined\n",
                PROJECTS_PATH, state->project, should_silkscreen_top ? "top" : "bottom",
                state->flatcam_options.offset_x, state->flatcam_options.offset_y);
    }

    char output[2048];
    sprintf(output, "open_gerber \"%s/%s/CAMOutputs/GerberFiles/profile.gbr\" -outname profile\n"
                    "offset profile %s %s\n"
                    "%s"
                    "cutout profile -dia 0.1 -margin -0.2 -gapsize 0.0 -gaps tb\n"
                    "\n"
                    "open_gerber \"%s/%s/CAMOutputs/GerberFiles/%s.gbr\" -outname copper_top\n"
                    "offset copper_top %s %s\n"
                    "%s"
                    "isolate copper_top -dia %s -passes 10 -overlap 1 -combine 1 -outname copper_top.iso\n"
                    "\n"
                    "join_geometries joined profile_cutout copper_top.iso\n"
                    "%s"
                    "cncjob %s -z_cut 0.0 -z_move 2.0 -feedrate %s -tooldia 0.2032\n"
                    "\n"
                    "open_excellon \"%s/%s/CAMOutputs/DrillFiles/drill_1_16.xln\" -outname drills\n"
                    "offset drills %s %s\n"
                    "%s"
                    "drillcncjob drills -drillz 0.3 -travelz 2.5 -feedrate 600.0 -tools 1 -outname check_holes_cnc\n"
                    "drillcncjob drills -drillz -3.0 -travelz 1.5 -feedrate 1000.0 -tools 1,2,3,4 -outname drill_holes_cnc\n"
                    "\n"
                    "write_gcode %s \"%s/%s/CAMOutputs/flatCAM/0_draw_copper_top.gcode\"\n"
                    "write_gcode check_holes_cnc \"%s/%s/CAMOutputs/flatCAM/1_check_holes_cnc.gcode\"\n"
                    "write_gcode drill_holes_cnc \"%s/%s/CAMOutputs/flatCAM/2_drill_holes_cnc.gcode\"\n"
                    "%s",
            PROJECTS_PATH, state->project,
            state->flatcam_options.offset_x,
            state->flatcam_options.offset_y,
            should_mirror ? "mirror profile -axis Y -box profile\n" : "",

            PROJECTS_PATH, state->project, traces_file,
            state->flatcam_options.offset_x,
            state->flatcam_options.offset_y,
            should_mirror ? "mirror copper_top -axis Y -box profile\n" : "",
            state->flatcam_options.dia_width,

            silkscreen_output,
            should_silkscreen ? "joined_1" : "joined", state->flatcam_options.feedrate_etch,

            PROJECTS_PATH, state->project,
            state->flatcam_options.offset_x,
            state->flatcam_options.offset_y,
            should_mirror ? "mirror drills -axis Y -box profile\n" : "",

            should_silkscreen ? "joined_1_cnc" : "joined_cnc", PROJECTS_PATH, state->project,
            PROJECTS_PATH, state->project,
            PROJECTS_PATH, state->project,
            should_mirror ? "" : "plot\n"
    );

    copy_to_clipboard(output);

    // Check if output folder exists
    char buffer[256];
    sprintf(buffer, "%s/%s/CAMOutputs/flatCAM", PROJECTS_PATH, state->project);
    struct stat st;
    if (stat(buffer, &st) != 0) {
        // Creating output directory
        mkdir(buffer, 0700);
    }
}

void flatcam_generate(AppState *state) {
    generate_script(state);
}

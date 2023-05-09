//
// Created by samuel on 22-9-22.
//

#include "checklist.h"

const char *checklist_checks[] = {
        "-- Computer --",
        "Export EAGLE files (disable Cutouts in Profile)",
        "Run FlatCAM scripts",
        "Modify GCode files",
        "-- Preparation --",
        "Mount Dremel tool on printer with knob facing X axis direction",
        "Saw the board",
        "Set clamp",
        "Clean board",
        "Cover the board with ink from a permanent marker",
        "Secure board in clamp",
        "Align print head Z-axis with board surface",
        "Adjust Z offset screw for Z-axis homing alignment with board surface",
        "Backup manual mesh by taking a photo of it",
        "Perform a manual mesh across board",
        "-- Print traces --",
        "Home all axis",
        "Reset Z-offset to 0.0",
        "Turn on Dremel to 14000 RPM",
        "Start 0_draw_traces print",
        "-- Etch --",
        "Turn printer off",
        "Remove board from clamp",
        "Gentle blow off any copper dust",
        "Put board in etching solution",
        "Remove board from etching solution",
        "Clean the board",
        "Remove the ink from the board",
        "-- Drilling --",
        "Start 1_pre_drill_holes print (optional, not recommended)",
        "Remove board from clamp",
        "Put tape on top of the board (copper side) (not the sides!)",
        "Change Dremel bit",
        "Secure board in clamp (copper side up)",
        "Align print head Z-axis with board surface",
        "Turn on the printer",
        "Home X and Y axis",
        "Start 2_check_holes_cnc print",
        "Adjust X axis screw for the holes",
        "Restart 2_check_holes print and repeat as needed",
        "Home X and Y axis",
        "Start 3_drill_holes print",
        "During the start of this print, set Z-offset to -3.0",
        "And remove Z-stop after homing",
        "Reset Z-offset to 0.0",
        "-- Silkscreen --",
        "Measure distance from clamp left side to left side of profile",
        "Make sure this distance is available on the right side of the board",
        "Flip over board and make sure the correct distance is maintained between the clamp and the profile",
        "Start 2_check_holes_cnc print if silkscreen is on copper side.",
        "Start 4_check_mirrored_holes_cnc print if silkscreen is on non-copper side",
        "Adjust X axis screw for the holes",
        "Restart X_check_holes print and repeat as needed",
        "Reset Z-offset to 0.0",
        "Turn on Dremel to 14000 RPM",
        "Start 5_draw_silkscreen print",
        "-- Finish --",
        "Turn off printer and Dremel tool",
        "Remove board from clamp",
        "Remove tape from board",
};

const int checklist_length = (sizeof(checklist_checks) / sizeof(const char *));
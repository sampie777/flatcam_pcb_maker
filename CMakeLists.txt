cmake_minimum_required(VERSION 3.23)
project(flatcam_pcb_maker C)

set(CMAKE_C_STANDARD 99)

add_executable(flatcam_pcb_maker main.c terminal_utils.c screen.c file_utils.c utils.c dialog.c flatcam_generator.c gcode_modifier.c checklist.c eagle_board_parser.c gnd_pads.c bed_leveling.c)
target_link_libraries(flatcam_pcb_maker m)

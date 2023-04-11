flatcam_pcb_maker: main.c
	$(CC) main.c terminal_utils.c screen.c file_utils.c utils.c dialog.c flatcam_generator.c gcode/gcode_modifier.c checklist.c eagle_board_parser.c gcode/gnd_pads.c leveling/leveling_utils.c leveling/bed_leveling.c bitmap.c leveling/height_calculation.c leveling/heightmap_image.c gcode/gcode_modifier_line_mapper.c -Wall -pedantic -std=c99 -D_BSD_SOURCE -o flatcam_pcb_maker.exe -lm
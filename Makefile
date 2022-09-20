flatcam_pcb_maker: main.c
	$(CC) main.c terminal_utils.c screen.c file_utils.c utils.c dialog.c flatcam_generator.c gcode_modifier.c -Wall -pedantic -std=c99 -D_BSD_SOURCE -o flatcam_pcb_maker.exe
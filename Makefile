kilo: main.c
	$(CC) main.c screen.c terminal_utils.c file_utils.c -o flatcam_pcb_maker -Wall -Wextra -pedantic -std=c99

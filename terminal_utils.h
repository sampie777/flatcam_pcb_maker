//
// Created by samuel on 16-9-22.
//

#ifndef FLATCAM_PCB_MAKER_TERMINAL_UTILS_H
#define FLATCAM_PCB_MAKER_TERMINAL_UTILS_H

#include <stdbool.h>

void clearScreen();

void die(const char *s);

void disableRawMode();

void enableRawMode();

int getCursorPosition(int *row, int *col);

int getWindowSize(int *rows, int *cols);

int editorReadKey(bool blocking);

#endif //FLATCAM_PCB_MAKER_TERMINAL_UTILS_H

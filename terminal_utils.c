//
// Created by samuel on 16-9-22.
//

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "terminal_utils.h"
#include "common.h"

void clearScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);  // Move cursor to 1,1
}

void die(const char *s) {
    clearScreen();

    perror(s);
    exit(1);
}

static struct termios original_termios_state;
void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios_state) == -1)
        die("Failed to disable raw mode");
}

void enableRawMode() {
    struct termios new_state;
    if (tcgetattr(STDIN_FILENO, &original_termios_state) == -1)
        die("Failed to get original terminal attributes");
    atexit(disableRawMode);

    // Shallow clone of the struct
    new_state = original_termios_state;

    // Turn off the terminal's default echo back of input characters: ECHO
    // Also enable direct character input instead of waiting for ENTER: ICANON
    // Disable Ctrl+C commands: ISIG
    // Disable Ctrl+V commands: IEXTEN
    // Disable Ctrl+S commands: IXON
    // Disable Ctrl+M commands: ICRNL
    // Disable output processing of \n and \r: OPOST
    new_state.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    new_state.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    new_state.c_oflag &= ~(OPOST);
    new_state.c_cflag |= CS8;

    new_state.c_cc[VMIN] = 0;   // Min bytes to read for read()
    new_state.c_cc[VTIME] = 1;  // Max timeout in 100 ms for read()

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_state) == -1)
        die("Failed to apply new terminal attributes");
}

int getCursorPosition(int *row, int *col) {
    char buff[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;

    while (i < sizeof(buff) - 1) {
        if (read(STDIN_FILENO, &buff[i], 1) != 1)
            break;
        if (buff[i] == 'R')
            break;
        i++;
    }
    buff[i] = '\0'; // Close string

    // Parse buffer
    if (buff[0] != '\x1b' || buff[1] != '[')
        return -1;
    if (sscanf(&buff[2], "%d;%d", row, col) != 2)
        return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return getCursorPosition(rows, cols);
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
}

int editorReadKey() {
    int nread;
    char c = '\0';
    while ((nread = (int) read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            die("Failed to read input");
    }

    if (c != '\x1b')
        return c;

    // Handle escape characters
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1)
        return c;
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
        return c;

    if (seq[0] == '[') {
        if (seq[1] > '0' && seq[1] < '9') {
            if (read(STDIN_FILENO, &seq[2], 1) != 1)
                return c;

            if (seq[2] != '~')
                return c;

            switch (seq[1]) {
                case '1':
                    return HOME_KEY;
                case '4':
                    return END_KEY;
                case '3':
                    return DELETE_KEY;
                case '5':
                    return PAGE_UP;
                case '6':
                    return PAGE_DOWN;
                case '7':
                    return HOME_KEY;
                case '8':
                    return END_KEY;
            }
        } else {
            switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
            }
        }
    } else if (seq[0] == 'O') {
        switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
        }
    }

    return c;
}
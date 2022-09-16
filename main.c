#include <stdio.h>

struct ScreenConfig {
    int row_count;
    int column_count;
};

/*** terminal ***/

void clearScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);  // Move cursor to 1,1
}

void die(const char *s) {
    clearScreen();

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios_state) == -1)
        die("Failed to disable raw mode");
}

void enableRawMode() {
    struct termios new_state;
    if (tcgetattr(STDIN_FILENO, &E.original_termios_state) == -1)
        die("Failed to get original terminal attributes");
    atexit(disableRawMode);

    // Shallow clone of the struct
    new_state = E.original_termios_state;

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

int main() {
    struct ScreenConfig screen = {};
    enableRawMode();

    if (getWindowSize(&screen.row_count, &screen.column_count) == -1)
        die("Failed to get window size");

    printf("Hello, World!\n");
    return 0;
}

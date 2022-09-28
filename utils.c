//
// Created by samuel on 16-9-22.
//

#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
#include <windows.h>
#endif

int bound_int(int value, int min, int max, bool roll_over) {
    if (max < min) return value;

    if (value < min) {
        if (roll_over)
            return max;
        return min;
    }
    if (value > max) {
        if (roll_over)
            return value % (max + 1);
        return max;
    }
    return value;
}

double bound_double(double value, double min, double max) {
    if (max < min) return value;

    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

void copy_to_clipboard(const char *data) {
    const size_t length = strlen(data) + 1;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, length);
    memcpy(GlobalLock(hMem), data, length);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
#elif __APPLE__
    const char proto_cmd[] = "echo '%s' | phcopy";
    char cmd[length + strlen(proto_cmd) - 2]; // -2 to remove the length of %s in proto cmd
    sprintf(cmd, proto_cmd, data);
    system(cmd);
#else
    const char proto_cmd[] = "echo '%s' | xclip -selection clipboard";
    char cmd[length + strlen(proto_cmd) - 2]; // -2 to remove the length of %s in proto cmd
    sprintf(cmd, proto_cmd, data);
    system(cmd);
#endif
}

void open_folder(const char *path) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(__CYGWIN__)
    const char proto_cmd[] = "start \"\" \"%s\"";
    char cmd[strlen(path) + strlen(proto_cmd)];
    sprintf(cmd, proto_cmd, path);
    system(cmd);
#elif __APPLE__
    const char proto_cmd[] = "open \"%s\"";
    char cmd[strlen(path) + strlen(proto_cmd)];
    sprintf(cmd, proto_cmd, path);
    system(cmd);
#else
    const char proto_cmd[] = "nautilus \"%s\" &";
    char cmd[strlen(path) + strlen(proto_cmd)];
    sprintf(cmd, proto_cmd, path);
    system(cmd);
#endif
}

bool starts_with(const char *source, const char *needle) {
    return strncmp(needle, source, strlen(needle)) == 0;
}

void string_replace(char *input, char needle, char replacement) {
    char *buffer = malloc(strlen(input) + 1);
    memset(buffer, '\0', strlen(input) + 1);
    for (int i = 0; i < strlen(input); i++) {
        if (input[i] == needle) {
            buffer[i] = replacement;
        } else {
            buffer[i] = input[i];
        }
    }
    strcpy(input, buffer);
}

void auto_format_double(double input, char **destination) {
    char buffer[64], output[64];
    sprintf(buffer, "%lf", input);
    memset(output, '\0', sizeof(output));

    bool non_zero_found = false;
    for (int i = (int) strlen(buffer) - 1; i >= 0; i--) {
        if (!non_zero_found) {
            if (buffer[i] == '0') continue;
            if (buffer[i] == '.') {
                non_zero_found = true;
                continue;
            }
            non_zero_found = true;
        }

        output[i] = buffer[i];
    }

    if (destination == NULL || *destination == NULL || strlen(*destination) < strlen(output)) {
        *destination = malloc(strlen(output) + 1);
    }
    strcpy(*destination, output);
}

void auto_format_double_string(char *input) {
    char output[64] = {0};
    bool non_zero_found = false;
    bool decimal_found = false;
    for (int i = (int) strlen(input) - 1; i >= 0; i--) {
        if (input[i] == '.') {
            decimal_found = true;
        }

        if (!non_zero_found) {
            if (input[i] == '0') continue;
            if (input[i] == '.') {
                non_zero_found = true;
                continue;
            }
            non_zero_found = true;
        }

        output[i] = input[i];
    }

    if (!decimal_found) {
        // Nothing to change
        return;
    }

    strcpy(input, output);
}
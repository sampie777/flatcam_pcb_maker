//
// Created by samuel on 16-9-22.
//

#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include <windows.h>
#endif

int bound(int value, int min, int max, bool roll_over) {
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

void copy_to_clipboard(const char *data) {
    const size_t length = strlen(data) + 1;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
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
    printf("=> Copied to clipboard!\n");
}
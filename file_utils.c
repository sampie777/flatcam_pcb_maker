//
// Created by samuel on 16-9-22.
//

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "file_utils.h"
#include "return_codes.h"

void files_get_all_projects(const char *path, char ***projects, int *projects_count) {
    *projects_count = 0;
    *projects = NULL;

    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type != DT_DIR) continue;
            if (strlen(dir->d_name) <= 0) continue;
            if (dir->d_name[0] == '.') continue;

            (*projects_count)++;
            *projects = realloc(*projects, *projects_count * sizeof(char *));
            (*projects)[*projects_count - 1] = malloc((strlen(dir->d_name) + 1) * sizeof(char));
            strcpy((*projects)[*projects_count - 1], dir->d_name);
        }
        closedir(d);
    }
}

int file_read_line(FILE *file, char **output) {
    if (file == NULL) return RESULT_FAILED;
    if (output == NULL) return RESULT_FAILED;
    if (feof(file)) return RESULT_EMPTY;

    int buffer_size = 128;
    int buffer_index = 0;
    char *buffer = malloc(buffer_size);

    while (!feof(file)) {
        char c = (char) fgetc(file);
        if (c == '\r') continue;
        if (c == '\n' || c == EOF) break;

        buffer[buffer_index++] = c;

        if (buffer_index > buffer_size) {
            buffer_size += 128;
            buffer = realloc(buffer, buffer_size);
        }
    }
    buffer[buffer_index] = '\0';

    *output = malloc(strlen(buffer) + 1);
    strcpy(*output, buffer);
    free(buffer);
    return RESULT_OK;
}

int copy_file(const char *old_file, const char *new_file) {
    FILE *old = fopen(old_file, "r");
    FILE *new = fopen(new_file, "w");
    if (old == NULL) return RESULT_FAILED;
    if (new == NULL) return RESULT_FAILED;

    char buffer[128];
    while (!feof(old)) {
        size_t length = fread(buffer, sizeof(char), sizeof(buffer), old);
        fwrite(buffer, sizeof(char), length, new);
    }

    fclose(old);
    fclose(new);
    return RESULT_OK;
}
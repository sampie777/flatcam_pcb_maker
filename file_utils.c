//
// Created by samuel on 16-9-22.
//

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "file_utils.h"

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
            (*projects)[*projects_count - 1] = malloc((strlen(dir->d_name) + 1) * sizeof (char));
            strcpy((*projects)[*projects_count - 1], dir->d_name);
            (*projects)[*projects_count - 1][strlen(dir->d_name)] = '\0';
        }
        closedir(d);
    }
}

//
// Created by samuel on 16-9-22.
//

#include <bits/types/FILE.h>

#ifndef FLATCAM_PCB_MAKER_FILE_UTILS_H
#define FLATCAM_PCB_MAKER_FILE_UTILS_H

void files_get_all_projects(const char *path, char ***projects, int *projects_count);

int file_read_line(FILE *file, char **output);

int copy_file(const char *old_file, const char *new_file);

#endif //FLATCAM_PCB_MAKER_FILE_UTILS_H

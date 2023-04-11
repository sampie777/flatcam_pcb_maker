//
// Created by samuel on 9-3-23.
//

#include <malloc.h>
#include "bitmap.h"


void bitmap_malloc(BitMap *bitmap) {
    if (bitmap->scale == 0)
        bitmap->scale = 1;

    bitmap->data = (char ***) malloc(sizeof(char **) * bitmap->height);
    for (int i = 0; i < bitmap->height; i++) {
        bitmap->data[i] = (char **) malloc(sizeof(char *) * bitmap->width);

        for (int j = 0; j < bitmap->width; j++) {
            bitmap->data[i][j] = (char *) calloc(3, sizeof(char));
        }
    }
}

void bitmap_save(const BitMap *bitmap, const char *filename) {
    FILE *file;
    int width = bitmap->scale * bitmap->width;
    int height = bitmap->scale * bitmap->height;
    int filesize = 54 + 3 * width * height;

    unsigned char file_header[14] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0};
    unsigned char info_header[40] = {40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0};
    unsigned char row_padding[3] = {0, 0, 0};

    file_header[2] = (unsigned char) (filesize);
    file_header[3] = (unsigned char) (filesize >> 8);
    file_header[4] = (unsigned char) (filesize >> 16);
    file_header[5] = (unsigned char) (filesize >> 24);
    info_header[4] = (unsigned char) width;
    info_header[5] = (unsigned char) (width >> 8);
    info_header[6] = (unsigned char) (width >> 16);
    info_header[7] = (unsigned char) (width >> 24);
    info_header[8] = (unsigned char) height;
    info_header[9] = (unsigned char) (height >> 8);
    info_header[10] = (unsigned char) (height >> 16);
    info_header[11] = (unsigned char) (height >> 24);

    file = fopen(filename, "wb");
    fwrite(file_header, 1, 14, file);
    fwrite(info_header, 1, 40, file);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Write in the order of blue, green, red
            fwrite(bitmap->data[i / bitmap->scale][j / bitmap->scale] + 2, sizeof(char), 1, file);
            fwrite(bitmap->data[i / bitmap->scale][j / bitmap->scale] + 1, sizeof(char), 1, file);
            fwrite(bitmap->data[i / bitmap->scale][j / bitmap->scale], sizeof(char), 1, file);
        }
        fwrite(row_padding, 1, (4 - (width * 3) % 4) % 4, file);
    }

    fclose(file);
}
//
// Created by samuel on 15-3-23.
//

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "heightmap_image.h"
#include "height_calculation.h"
#include "../screen.h"
#include "../bitmap.h"
#include "utils_color.h"


double get_height_for_point(const Leveling *leveling, Point3D *point) {
    return bilinear_interpolation(leveling, point);
}

void factor_to_height_color(double factor, char *r, char *g, char *b) {
    RGB rgb = hsl2rgb((1 - factor) * 0.85 - 0.15, 1, 0.5);
    *r = rgb.r;
    *g = rgb.g;
    *b = rgb.b;
}

/**
 * Create and save the heightmap as an image
 * @param leveling
 */
void leveling_create_bitmap(Leveling *leveling, const char *filename) {
    // The margin outside of the measurement points
    int offsetX = 2;
    int offsetY = 2;

    // Get minZ/maxZ X and Y points
    int min_x = 9999;
    int max_x = -9999;
    int min_y = 9999;
    int max_y = -9999;
    for (int i = 0; i < leveling->row_length; i++) {
        for (int j = 0; j < leveling->column_length; j++) {
            Point3D *point = &(leveling->measurements[i][j]);
            if (point->x < min_x) min_x = (int) point->x;
            if (point->x > max_x) max_x = (int) point->x;
            if (point->y < min_y) min_y = (int) point->y;
            if (point->y > max_y) max_y = (int) point->y;
        }
    }

    // This image we will edit
    BitMap bitmap = {
            .width = (max_x - min_x) + 2 * offsetX + 1 + 2,
            .height = (max_y - min_y) + 2 * offsetY + 1,
            .scale = 10,
    };
    bitmap_malloc(&bitmap);

    // Get the minZ/maxZ Z
    Point3D point = {0};
    double maxZ = -9999;
    double minZ = 9999;
    for (int i = 0; i < bitmap.height; i++) {
        for (int j = 0; j < bitmap.width; j++) {
            point.x = min_x + j - offsetX;
            point.y = min_y + i - offsetY;
            double result = get_height_for_point(leveling, &point);
            if (result > maxZ) maxZ = result;
            if (result < minZ) minZ = result;
        }
    }

    // Create each pixel for the image
    for (int i = 0; i < bitmap.height; i++) {
        for (int j = 0; j < bitmap.width; j++) {
            char *r = &(bitmap.data[i][j][0]);
            char *g = &(bitmap.data[i][j][1]);
            char *b = &(bitmap.data[i][j][2]);

            // Draw spectrum on the right
            if (j == bitmap.width - 2) {
                *r = 0;
                *g = 0;
                *b = 0;
                continue;
            } else if (j == bitmap.width - 1) {
                double factor = (double) i / bitmap.height;
                factor_to_height_color(factor, r, g, b);
                continue;
            }

            point.x = min_x + j - offsetX;
            point.y = min_y + i - offsetY;

            // Color measurement points
            bool pixel_calculated = false;
            for (int row = 0; row < leveling->row_length; row++) {
                for (int col = 0; col < leveling->column_length; col++) {
                    Point3D *current_point = &(leveling->measurements[row][col]);
                    if (current_point->x == point.x && current_point->y == point.y) {
                        if (row == 0 && col == 0) {
                            r = 0;
                            g = 0;
                            b = 0;
                            pixel_calculated = true;
                        } else {
                            *r = (char) 255;
                            *g = (char) 255;
                            *b = (char) 255;
                            pixel_calculated = true;
                        }
                    }
                }
            }

            if (pixel_calculated) continue;

            // Color approximate pixel height
            double height = get_height_for_point(leveling, &point);
            double factor = (height - minZ) / (maxZ - minZ);
            factor_to_height_color(factor, r, g, b);
        }
    }

    bitmap_save(&bitmap, filename);
    free(bitmap.data);
}

char height_factor_to_static_color(double height_factor) {
    int factor = (int) (height_factor * 6);
    switch (factor) {
        case 0:
            return 4;   // Blue
        case 1:
            return 6;   // Cyan
        case 2:
            return 2;   // Green
        case 3:
            return 3;   // Yellow
        case 4:
            return 1;   // Red
        case 5:
        case 6:
            return 5;   // Purple
        default:
            return 9;
    }
}

void leveling_create_terminal_image(Leveling *leveling, int width, char **output) {
    if (leveling->row_length <= 1 || leveling->column_length <= 1) return;

    // Get minZ/maxZ X and Y points
    int min_x = 9999;
    int max_x = -9999;
    int min_y = 9999;
    int max_y = -9999;
    for (int i = 0; i < leveling->row_length; i++) {
        for (int j = 0; j < leveling->column_length; j++) {
            Point3D *point = &(leveling->measurements[i][j]);
            if (point->x < min_x) min_x = (int) point->x;
            if (point->x > max_x) max_x = (int) point->x;
            if (point->y < min_y) min_y = (int) point->y;
            if (point->y > max_y) max_y = (int) point->y;
        }
    }
    int board_width = max_x - min_x;
    int board_height = max_y - min_y;

    if (board_width == 0 || board_height == 0) return;

    // Get the minZ/maxZ Z
    Point3D point = {0};
    double max = -9999;
    double min = 9999;
    for (int i = 0; i < board_height; i++) {
        for (int j = 0; j < board_width; j++) {
            point.x = min_x + j;
            point.y = min_y + i;
            double result = get_height_for_point(leveling, &point);
            if (result > max) max = result;
            if (result < min) min = result;
        }
    }

    int canvas_width = width;
    int canvas_height = (int) (0.37 * board_height * ((double) canvas_width / board_width));

    *output = malloc((canvas_width + 1) * (5 + 2 + 4 + 5) * (canvas_height + 1) * sizeof(char) + canvas_height * sizeof(char));
    *output[0] = '\0';
    char buffer[32];
    for (int i = canvas_height; i >= 0; i--) {
        strcat(*output, " ");
        for (int j = 0; j < canvas_width + 1; j++) {
            point.x = min_x + board_width * ((double) j / canvas_width);
            point.y = min_y + board_height * ((double) i / canvas_height);

            double height = get_height_for_point(leveling, &point);
            double factor = (max - min) == 0 ? 0 : (height - min) / (max - min);
            char pixel = height_factor_to_static_color(factor);

            bool solid = false;
            for (int r = 0; r < leveling->row_length; r++) {
                for (int c = 0; c < leveling->column_length; c++) {
                    Point3D *measurement_point = &(leveling->measurements[r][c]);
                    if (measurement_point->x >= point.x
                        && measurement_point->x <= min_x + board_width * ((double) (j + 1) / canvas_width)
                        && measurement_point->y >= point.y
                        && measurement_point->y <= min_y + board_height * ((double) (i + 1) / canvas_height)) {
                        solid = true;
                    }
                }
            }

            sprintf(buffer, "\x1b[3%dm%s", pixel, solid ? "█" : "▓");
            strcat(*output, buffer);
        }
        strcat(*output, SCREEN_COLOR_RESET""NEW_LINE);
    }
}
//
// Created by samuel on 15-3-23.
//

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include "heightmap_image.h"
#include "height_calculation.h"
#include "../screen.h"
#include "../bitmap.h"


double get_height_for_point(const Leveling *leveling, Point3D *point) {
    return bilinear_interpolation(leveling, point);
}

void factor_to_height_color(double factor, char *r, char *g, char *b) {
    if (factor > 0.5) {
        *r = (char) (255 * sin((factor - 0.5) * 2 * 0.5 * M_PI));
        *g = (char) (255 * cos((factor - 0.5) * 2 * 0.5 * M_PI));
        *b = 0;
    } else {
        *r = 0;
        *g = (char) (255 * sin((factor) * 2 * 0.5 * M_PI));
        *b = (char) (255 * cos((factor) * 2 * 0.5 * M_PI));
    }
}

/**
 * Create and save the heightmap as an image
 * @param leveling
 */
void create_bitmap(Leveling *leveling) {
    printf("\n"SCREEN_COLOR_CYAN"Generating image..."SCREEN_COLOR_RESET"\n");

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

            // Color center points
            for (int c = 0; c < leveling->center_points_length; c++) {
                Point3D *current_point = &(leveling->center_points[c]);
                if (round(current_point->x) == round(point.x) && round(current_point->y) == round(point.y)) {
                    double factor = (current_point->z - minZ) / (maxZ - minZ);
                    factor_to_height_color(factor, r, g, b);
                    *b = 110;
                    pixel_calculated = true;
                }
            }

            if (pixel_calculated) continue;

            // Color approximate pixel height
            double height = get_height_for_point(leveling, &point);
            double factor = (height - minZ) / (maxZ - minZ);
            factor_to_height_color(factor, r, g, b);
        }
    }

    bitmap_save(&bitmap, "img.bmp");
    free(bitmap.data);
}

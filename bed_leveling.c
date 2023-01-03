//
// Created by samuel on 2-1-23.
//

#include <stdlib.h>
#include "bed_leveling.h"

void bed_leveling_calculate_printer_leveling_points(const Plane3D *plane,
                                                    int mesh_size,
                                                    double mesh_x_min,
                                                    double mesh_x_max,
                                                    double mesh_y_min,
                                                    double mesh_y_max,
                                                    Point3D ***level_points) {
    *level_points = malloc(mesh_size * sizeof(Point3D *));
    for (int i = 0; i < mesh_size; i++) {
        (*level_points)[i] = malloc(mesh_size * sizeof(Point3D));
    }

    for (int row = 0; row < mesh_size; row++) {
        double y = mesh_y_min + row * (mesh_y_max - mesh_y_min);
        for (int col = 0; col < mesh_size; col++) {
            double x = mesh_x_min + col * (mesh_x_max - mesh_x_min);
            // Calculate z by:
            // ax + by + cz + d = 0  ->  z = -(ax + by + d) / c
            (*level_points)[row][col] = (Point3D) {
                    .x = x,
                    .y = y,
                    .z = plane->c == 0 ? 0 : -1 * (plane->a * x + plane->b * y + plane->d) / plane->c,
            };
        }
    }
}

void bed_leveling_calculate_plane(const Point3D *measure0,
                                  const Point3D *measure1,
                                  const Point3D *measure2,
                                  Plane3D *plane) {
    Point3D vector0 = {
            .x = measure0->x - measure1->x,
            .y = measure0->y - measure1->y,
            .z = measure0->z - measure1->z,
    };
    Point3D vector1 = {
            .x = measure0->x - measure2->x,
            .y = measure0->y - measure2->y,
            .z = measure0->z - measure2->z,
    };

    Point3D normal_vector = {
            .x = vector0.y * vector1.z - vector0.z * vector1.y,
            .y = vector0.z * vector1.x - vector0.x * vector1.z,
            .z = vector0.x * vector1.y - vector0.y * vector1.x,
    };

    plane->a = normal_vector.x;
    plane->b = normal_vector.y;
    plane->c = normal_vector.z;
    plane->d = -1 * (plane->a * measure0->x + plane->b * measure0->y + plane->c * measure0->z);
}

void bed_leveling_calculate(AppState *state) {
    bed_leveling_calculate_plane(
            &(state->printer.measure0),
            &(state->printer.measure1),
            &(state->printer.measure2),
            &(state->printer.plane)
    );
}
#ifndef __RAYTRACING_H
#define __RAYTRACING_H

#include "objects.h"
#include <stdint.h>

typedef struct ray {
    uint8_t *pixels;
    double *background_color;
    rectangular_node rectangulars;
    sphere_node spheres;
    light_node lights;
    const viewpoint *view;
    int width;
    int height;
    int id;
    int case_num;
} rays;

void raytracing(void *r);
#endif

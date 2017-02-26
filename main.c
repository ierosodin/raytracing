#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "primitives.h"
#include "raytracing.h"

#define OUT_FILENAME "out.ppm"

#define ROWS 512
#define COLS 512

static void write_to_ppm(FILE *outfile, uint8_t *pixels,
                         int width, int height)
{
    fprintf(outfile, "P6\n%d %d\n%d\n", width, height, 255);
    fwrite(pixels, 1, height * width * 3, outfile);
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

int main(int argc, char* argv[])
{
    int THREAD_NUM = 128;

    if (argc > 1)
        THREAD_NUM = atoi(argv[1]);

    uint8_t *pixels;
    light_node lights = NULL;
    rectangular_node rectangulars = NULL;
    sphere_node spheres = NULL;
    color background = { 0.0, 0.1, 0.1 };
    struct timespec start, end;

#include "use-models.h"

    /* allocate by the given resolution */
    pixels = malloc(sizeof(unsigned char) * ROWS * COLS * 3);
    if (!pixels) exit(-1);

    printf("# Rendering scene\n");
    /* do the ray tracing with the given geometry */
    clock_gettime(CLOCK_REALTIME, &start);

    pthread_t *id = (pthread_t *) malloc(THREAD_NUM * sizeof(pthread_t));
    rays **ptr = (rays **) malloc(THREAD_NUM * sizeof(rays *));
    for (int i = 0; i < THREAD_NUM; i++) {
        ptr[i] = (rays *) malloc(sizeof(rays));
        ptr[i]->pixels = pixels;
        ptr[i]->background_color = background;
        ptr[i]->rectangulars = rectangulars;
        ptr[i]->spheres = spheres;
        ptr[i]->lights = lights;
        ptr[i]->view = &view;
        ptr[i]->width = ROWS;
        ptr[i]->height = COLS;
        ptr[i]->id = i;
        ptr[i]->thread_num = THREAD_NUM;
        pthread_create(&id[i], NULL, (void *) &raytracing, (void *) ptr[i]);
    }

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(id[i], NULL);

    clock_gettime(CLOCK_REALTIME, &end);
    {
        FILE *outfile = fopen(OUT_FILENAME, "wb");
        write_to_ppm(outfile, pixels, ROWS, COLS);
        fclose(outfile);
    }

    for (int i = 0; i < THREAD_NUM; i++)
        free(ptr[i]);
    free(ptr);

    delete_rectangular_list(&rectangulars);
    delete_sphere_list(&spheres);
    delete_light_list(&lights);
    free(pixels);
    printf("Done!\n");
    printf("Execution time of raytracing() : %lf sec\n", diff_in_second(start, end));

    FILE *output = fopen("pthread.txt", "a");
    fprintf(output, "%d %lf\n", THREAD_NUM, diff_in_second(start, end));
    fclose(output);

    return 0;
}

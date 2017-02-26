#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "primitives.h"
#include "raytracing.h"
#include "threadpool.h"

#define OUT_FILENAME "out.ppm"

#define ROWS 512
#define COLS 512

static int THREAD_NUM = 12;
static int CASE_SPLIT = 512;
static tpool_t *pool = NULL;

struct {
    pthread_mutex_t mutex;
    int cut_THREAD_NUM;
} data_context;

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

static void *task_run(void *data)
{
    (void) data;
    while(1) {
        task_t *_task = tqueue_pop(pool->queue);
        if (_task) {
            if (!_task->func) {
                tqueue_push(pool->queue, _task);
                break;
            } else {
                _task->func(_task->arg);
                free(_task);
            }
        }
    }
    pthread_exit(NULL);
}

task_t *new_task()
{
    task_t *_task = (task_t *) malloc(sizeof(task_t));
    return _task;
}

int main(int argc, char* argv[])
{
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

    pthread_mutex_init(&(data_context.mutex), NULL);
    data_context.cut_THREAD_NUM = 0;
    pool = (tpool_t *) malloc(sizeof(tpool_t));
    tpool_init(pool, THREAD_NUM, task_run);

    rays **ptr = (rays **) malloc(CASE_SPLIT * sizeof(rays *));
    for (int i = 0; i < CASE_SPLIT; i++) {
        task_t *_task = (task_t *) malloc(sizeof(task_t));
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
        ptr[i]->case_num = CASE_SPLIT;
        _task->arg = ptr[i];
        _task->func = raytracing;
        tqueue_push(pool->queue, _task);
    }

    task_t *_task = (task_t *) malloc(sizeof(task_t));
    _task->func = NULL;
    tqueue_push(pool->queue, _task);

    tpool_free(pool);

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
    return 0;
}

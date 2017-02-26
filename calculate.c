#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    FILE *fp = fopen("pthread.txt", "r");
    FILE *output = fopen("output.txt", "w");
    if (!fp) {
        printf("ERROR opening input file pthread.txt\n");
        exit(0);
    }
    int i = 0, j = 0;
    int thread_num;
    for (i = 0; i < 10; i++) {
        double pthread_sum_r = 0.0, pthread_r;
        for (j = 0; j < 100; j++) {
            if (feof(fp)) {
                printf("ERROR: You need 100 datum instead of %d\n", j);
                printf("run 'make run' longer to get enough information\n\n");
                exit(0);
            }
            fscanf(fp, "%d %lf\n", &thread_num, &pthread_r);
            pthread_sum_r += pthread_r;
        }
        fprintf(output, "%d %lf\n",thread_num, pthread_sum_r / 100.0);
    }
    fclose(output);
    fclose(fp);
    return 0;
}

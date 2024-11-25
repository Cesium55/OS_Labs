#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include "sum_lib.h"

// Функция для генерации массива
void GenerateArray(int *array, int size, int seed) {
    srand(seed);
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 100; // Генерация чисел от 0 до 99
    }
}

void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
    // Парсинг аргументов командной строки
    int threads_num = -1;
    int array_size = -1;
    int seed = -1;

    while (1) {
        static struct option options[] = {
            {"threads_num", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"seed", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (option_index) {
            case 0:
                threads_num = atoi(optarg);
                break;
            case 1:
                array_size = atoi(optarg);
                break;
            case 2:
                seed = atoi(optarg);
                break;
            default:
                printf("Unknown option\n");
                return 1;
        }
    }

    if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    // Генерация массива
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        perror("Failed to allocate memory");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    // Создание потоков
    pthread_t threads[threads_num];
    struct SumArgs args[threads_num];

    int chunk_size = array_size / threads_num;
    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * chunk_size;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
    }

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < threads_num; i++) {
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
            printf("Error: pthread_create failed!\n");
            free(array);
            return 1;
        }
    }

    int total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("Total: %d\n", total_sum);
    printf("Elapsed time: %.2f ms\n", elapsed_time);

    free(array);
    return 0;
}

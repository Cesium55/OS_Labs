#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids; // Глобальный массив для хранения PID дочерних процессов.
int pnum;          // Число дочерних процессов (глобальная для обработчика сигналов).

// Обработчик сигнала для таймаута
void timeout_handler(int signum) {
    printf("Timeout exceeded! Killing all child processes.\n");
    for (int i = 0; i < pnum; i++) {
        kill(child_pids[i], SIGKILL);
    }
    free(child_pids);
    //exit(1);
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    pnum = -1; // Убираем из локальной области.
    bool with_files = false;
    int timeout = -1;  // Таймаут в секундах.

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {{"seed", required_argument, 0, 0},
                                          {"array_size", required_argument, 0, 0},
                                          {"pnum", required_argument, 0, 0},
                                          {"by_files", no_argument, 0, 'f'},
                                          {"timeout", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("Seed must be a positive integer\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("Array size must be a positive integer\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("Number of processes must be a positive integer\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    case 4:
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("Timeout must be a positive integer\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
               argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int pipefd[2 * pnum];
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipefd + 2 * i) == -1) {
                perror("pipe failed");
                return 1;
            }
        }
    }

    child_pids = malloc(sizeof(pid_t) * pnum);

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            // successful fork
            child_pids[i] = child_pid;
            if (child_pid == 0) {
                // child process
                struct MinMax min_max;
                int start = i * array_size / pnum;
                int end = (i + 1) * array_size / pnum;
                min_max = GetMinMax(array, start, end);

                if (with_files) {
                    char filename[256];
                    sprintf(filename, "output_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    fprintf(file, "%d %d\n", min_max.min, min_max.max);
                    fclose(file);
                } else {
                    close(pipefd[2 * i]);
                    write(pipefd[2 * i + 1], &min_max, sizeof(min_max));
                    close(pipefd[2 * i + 1]);
                }
                free(array);
                return 0;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    if (timeout > 0) {
        signal(SIGALRM, timeout_handler); // Устанавливаем обработчик сигнала.
        alarm(timeout);                  // Устанавливаем таймер.
    }

    int active_child_processes = pnum;
    while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            char filename[256];
            sprintf(filename, "output_%d.txt", i);
            FILE *file = fopen(filename, "r");
            fscanf(file, "%d %d", &min, &max);
            fclose(file);
        } else {
            close(pipefd[2 * i + 1]);
            struct MinMax local_min_max;
            read(pipefd[2 * i], &local_min_max, sizeof(local_min_max));
            close(pipefd[2 * i]);
            min = local_min_max.min;
            max = local_min_max.max;
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}

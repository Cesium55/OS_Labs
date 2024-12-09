#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>


#include <ctype.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <getopt.h>




#include "common.h" // Подключаем общий файл с MultModulo и структурой Server






struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result; // Сохраняем результат вычислений
};

void *ServerTask(void *args) {
    struct ThreadData *data = (struct ThreadData *)args;

    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
        pthread_exit(NULL);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(data->server.port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    // Формируем задачу
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive from %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    // Сохраняем результат
    memcpy(&data->result, response, sizeof(uint64_t));
    close(sck);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file[255] = {'\0'};

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {{"k", required_argument, 0, 0},
                                          {"mod", required_argument, 0, 0},
                                          {"servers", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                ConvertStringToUI64(optarg, &k);
                break;
            case 1:
                ConvertStringToUI64(optarg, &mod);
                break;
            case 2:
                memcpy(servers_file, optarg, strlen(optarg));
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;
        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == -1 || mod == -1 || !strlen(servers_file)) {
        fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // Чтение серверов из файла
    FILE *file = fopen(servers_file, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", servers_file);
        return 1;
    }

    unsigned int servers_num = 0;
    struct Server *servers = malloc(sizeof(struct Server) * 255); // Максимум 255 серверов
    while (fscanf(file, "%254s %d", servers[servers_num].ip, &servers[servers_num].port) != EOF) {
        servers_num++;
    }
    fclose(file);

    // Разделяем диапазон работы между серверами
    pthread_t threads[servers_num];
    struct ThreadData thread_data[servers_num];

    uint64_t range_per_server = k / servers_num;
    for (unsigned int i = 0; i < servers_num; i++) {
        thread_data[i].server = servers[i];
        thread_data[i].begin = i * range_per_server + 1;
        thread_data[i].end = (i == servers_num - 1) ? k : (i + 1) * range_per_server;
        thread_data[i].mod = mod;

        pthread_create(&threads[i], NULL, ServerTask, (void *)&thread_data[i]);
    }

    // Объединение результатов
    uint64_t total_result = 1;
    for (unsigned int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
        total_result = MultModulo(total_result, thread_data[i].result, mod);
    }

    printf("Final result: %llu\n", total_result);

    free(servers);
    return 0;
}

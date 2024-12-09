#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>


#include <ctype.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <getopt.h>



#include "common.h" // Подключаем общий файл с MultModulo и структурой Server

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t result = 1;
    for (uint64_t i = args->begin; i <= args->end; i++) {
        result = MultModulo(result, i, args->mod);
    }
    return result;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t *result = malloc(sizeof(uint64_t));
    if (!result) {
        fprintf(stderr, "Failed to allocate memory for result\n");
        pthread_exit(NULL);
    }
    *result = Factorial(fargs);
    pthread_exit(result);
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {{"port", required_argument, 0, 0},
                                          {"tnum", required_argument, 0, 0},
                                          {0, 0, 0, 0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                port = atoi(optarg);
                break;
            case 1:
                tnum = atoi(optarg);
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;
        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Usage: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Failed to create server socket\n");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Failed to bind to socket\n");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        fprintf(stderr, "Failed to listen on socket\n");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d\n", port);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            fprintf(stderr, "Failed to accept new connection\n");
            continue;
        }

        unsigned int buffer_size = sizeof(uint64_t) * 3;
        char from_client[buffer_size];
        int read = recv(client_fd, from_client, buffer_size, 0);

        if (read != buffer_size) {
            fprintf(stderr, "Received incorrect data format\n");
            close(client_fd);
            continue;
        }

        uint64_t begin = 0, end = 0, mod = 0;
        memcpy(&begin, from_client, sizeof(uint64_t));
        memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
        memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

        printf("Received task: %llu-%llu mod %llu\n", begin, end, mod);

        pthread_t threads[tnum];
        struct FactorialArgs args[tnum];
        uint64_t range = (end - begin + 1) / tnum;
        uint64_t remainder = (end - begin + 1) % tnum;

        for (int i = 0; i < tnum; i++) {
            args[i].begin = begin + i * range;
            args[i].end = (i == tnum - 1) ? end : args[i].begin + range - 1;
            if (i == tnum - 1 && remainder) args[i].end += remainder;
            args[i].mod = mod;

            if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i]) != 0) {
                fprintf(stderr, "Failed to create thread %d\n", i);
                close(client_fd);
                close(server_fd);
                return 1;
            }
        }

        uint64_t total_result = 1;
        for (int i = 0; i < tnum; i++) {
            uint64_t *thread_result;
            pthread_join(threads[i], (void **)&thread_result);
            total_result = MultModulo(total_result, *thread_result, mod);
            free(thread_result);
        }

        printf("Calculated result: %llu\n", total_result);

        char to_client[sizeof(total_result)];
        memcpy(to_client, &total_result, sizeof(total_result));
        if (send(client_fd, to_client, sizeof(total_result), 0) < 0) {
            fprintf(stderr, "Failed to send result to client\n");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

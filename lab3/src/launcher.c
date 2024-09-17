#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Starting sequential_min_max in child process...\n");


        char *exec_args[argc + 1];
        exec_args[0] = "./sequential_min_max";
        for (int i = 1; i < argc; i++) {
            exec_args[i] = argv[i];
        }
        exec_args[argc] = NULL;

        execv(exec_args[0], exec_args);

        return 1;
    } else {
        wait(NULL);
    }

    return 0;
}

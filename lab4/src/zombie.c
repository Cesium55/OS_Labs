#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid > 0) {
        // Родительский процесс: намеренно не вызывает wait()
        printf("Parent process (PID=%d)\n", getpid());
        printf("Check the process table for a zombie process.\n");

        // Бесконечный цикл, чтобы родитель продолжал работать
        for(int i=0;i<20;i++) {
            sleep(1);
            
            if (i == 10){
		int status;
                wait(&status);
            }
        }


        

    } else if (pid == 0) {
        // Дочерний процесс: завершение
        printf("Child process (PID=%d): I'm exiting.\n", getpid());
        exit(0);
    } else {
        perror("Fork failed");
        return 1;
    }

    return 0;
}

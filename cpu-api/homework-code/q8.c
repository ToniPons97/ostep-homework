#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int pipefd[2];
    char buf;

    if (pipe(pipefd) == -1) {
        perror("Error creating pipe\n");
        return 1;
    }

    pid_t w_pid = fork();
    if (w_pid < 0) {
        perror("fork");
        exit(1);
    }

    if (w_pid == 0) {
        const char* message = "WRITER IS TALKING\n";
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        write(STDOUT_FILENO, message, strlen(message));

        close(pipefd[1]);
        _exit(0);
    }

    pid_t r_pid = fork();
    if (r_pid < 0) {
        perror("fork");
        exit(1);
    }

    if (r_pid == 0) {
        close(pipefd[1]);

        dup2(pipefd[0], STDIN_FILENO);

        while (read(STDIN_FILENO, &buf, 1) > 0)
            write(STDOUT_FILENO, &buf, 1);

        close(pipefd[0]);
        _exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(w_pid, NULL, 0);
    waitpid(r_pid, NULL, 0);

    return 0;
}

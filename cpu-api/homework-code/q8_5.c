#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("Example: %s <file>\n", argv[0]);
        _exit(0);
    }

    const char* filename = argv[1];
    int pipefd[2];

    if (pipe(pipefd) == -1) {
        perror("Error creating pipe");
        _exit(1);
    }

    pid_t w_pid = fork();
    if (w_pid < 0) {
        perror("fork");
        _exit(1);
    }

    if (w_pid == 0) {
        int res = dup2(pipefd[1], STDOUT_FILENO);
        if (res == -1) {
            perror("dup2");
            _exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execlp("cat", "cat", filename, (char*)NULL);
        perror("execlp cat");
        _exit(1);
    }

    pid_t r_pid = fork();
    if (r_pid < 0) {
        perror("fork");
        _exit(1);
    }

    if (r_pid == 0) {
        int res = dup2(pipefd[0], STDIN_FILENO);
        if (res == -1) {
            perror("dup2");
            _exit(1);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execlp("wc", "wc", "-l", (char*)NULL);
        perror("execlp wc");
        _exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(w_pid, NULL, 0);
    waitpid(r_pid, NULL, 0);

    return 0;
}

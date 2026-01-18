#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {

    int rc = fork();

    if (rc < 0) {
        perror("error cloning process\n");
        return 1;
    } else if (rc == 0) {
        close(STDOUT_FILENO);
        printf("(pid: %d) Hello from child process\n", getpid());
    } else {
        printf("(pid: %d) running parent process\n", getpid());
        int wc = wait(NULL);
        printf("waited for child process: %d\n", wc);
    }

    return 0;
}
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int rc = fork();

    if (rc < 0) {
        perror("Error cloning process\n");
        return 1;
    } else if (rc == 0) {
        printf("(pid: %d) child is executing\n", getpid());

        int wc = wait(NULL);
        if (wc < 0) {
            perror("Error waiting\n");
        }

        printf("(pid: %d) waited for %d\n", getpid(), wc);
    } else {
        printf("(pid: %d) parent is executing\n", getpid());
        int wc = wait(NULL);
        printf("(pid: %d) waited for %d\n", getpid(), wc);
    }

    return 0;
}
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int a = 100;
    int rc = fork();

    if (rc < 0) {
        perror("Error cloning process\n");
        return 1;
    } else if (rc == 0) {
        a += 10;
        printf("(pid: %d) child running\n", getpid());
        printf("a = %d\n", a);
        return 0;
    } else {
        a += 1;
        printf("(pid: %d) parent running\n", getpid());
        wait(NULL);
        printf("(pid: %d) parent running\n", getpid());
        printf("a = %d\n", a);
    }

    printf("(pid: %d) (outside of if else) a = %d\n", getpid(), a);
    return 0;
}
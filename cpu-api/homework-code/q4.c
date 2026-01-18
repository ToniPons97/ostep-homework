#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int rc = fork();

    if (rc < 0) {
        perror("Error cloning process\n");
        return 1;
    } else if (rc == 0) {
        printf("(pid: %d) running child\n", getpid());
        execle("/bin/ls", "/bin/ls", "-al", (char*)NULL, NULL);
    }

    return 0;
}
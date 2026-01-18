#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int rc = fork();

    if (rc < 0) {
        perror("Error cloning process\n");
        return 1;
    } else if (rc == 0) {
        printf("Hello\n");
    } else {
        printf("Goodbye\n");
    }

    return 0;
}
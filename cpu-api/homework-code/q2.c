#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int fd = open("test.txt", O_CREAT | O_RDWR | O_APPEND, 0770);

    if (fd < 0) {
        perror("Error creating / opening file\n");
        return 1;
    }

    int rc = fork();
    if (rc < 0) {
        perror("error cloning process\n");
        return 1;
    } else if (rc == 0) {
        char text[] = "\nChild was here";
        write(fd, text, sizeof(text) - 1);
    } else {
        char text[] = "\nParent was here";
        // wait(NULL);
        write(fd, text, sizeof(text) - 1);
    }

    close(fd);

    return 0;
}
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
    } else {
        pid_t parent_pid = getpid();
        printf("(pid: %d) parent is executing\n", parent_pid);

        int dead_child_status = 0;
        int wc = waitpid(rc, &dead_child_status, 0);

        printf("(pid: %d) waited for %d\n", parent_pid, wc);
        printf("(pid: %d) dead child status: %d\n", parent_pid,
               dead_child_status);

        if (WIFEXITED(dead_child_status)) {
            printf("child exited with status %d\n",
                   WEXITSTATUS(dead_child_status));
        }
    }

    return 0;
}
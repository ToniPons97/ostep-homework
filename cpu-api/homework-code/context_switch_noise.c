#define _GNU_SOURCE

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

static void pin_to_cpu_core(int cpu_core);
static void create_pipe(int pipe[2]);
static void create_child_process(int* pid_out);

int main(int argc, char** argv)
{
    pin_to_cpu_core(19);
    
    int a_to_b[2], b_to_a[2];
    int pid_a, pid_b;

    create_pipe(a_to_b);
    create_pipe(b_to_a);

    create_child_process(&pid_a);
    
    if (pid_a == 0) {
        const char*  send = "abcd";
        char recv; 

        close(a_to_b[0]);
        close(b_to_a[1]);

        if (write(a_to_b[1], send++, 1) != 1)
            _exit(1);
            
        while (read(b_to_a[0], &recv, 1) > 0) {            
            printf("(pid_a) read from pipe_b: %c\n", recv);    
            
            if (write(a_to_b[1], send++, 1) != 1)
                _exit(1);
        }
        
        close(a_to_b[1]);
        close(b_to_a[0]);

        _exit(0);
    }

    create_child_process(&pid_b);

    if (pid_b == 0) {
        const char* send = "1234";
        char recv;

        close(a_to_b[1]);
        close(b_to_a[0]);

        while (read(a_to_b[0], &recv, 1) > 0) {            
            printf("(pid_b) read from pipe_a: %c\n", recv);
            
            if (write(b_to_a[1], send++, 1) != 1)
                _exit(1);

            if (*send == '\0')
                break;
        }

        close(a_to_b[0]);
        close(b_to_a[1]);

        _exit(0);
    }

    for (int i = 0; i < 2; i++) {
        close(a_to_b[i]);
        close(b_to_a[i]);
    }

    waitpid(pid_a, NULL, 0);
    waitpid(pid_b, NULL, 0);

    return 0;
}

static void pin_to_cpu_core(int cpu_core) 
{
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu_core, &set);
    
    if (sched_setaffinity(0, sizeof(set), &set) == -1) {
        perror("sched_setaffinity");
        _exit(1);
    }
}

static void create_pipe(int pipe[2])
{
    if (pipe2(pipe, 0) == -1) {
        perror("pipe");
        _exit(1);
    }
}

static void create_child_process(int* pid_out)
{
    *pid_out = fork();
    if (*pid_out == -1) {
        perror("fork");
        _exit(1);
    }
}
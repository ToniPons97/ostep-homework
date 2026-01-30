/*
    This program aims to measure Ping-pong IPC latency under forced scheduling

    - scheduler fairness
    - pipe latency
    - kernel wakeups
    - cache bouncing

    --cpu flag
        Pins parent and children to the same cpu core.
    
    With pinning:
        
        Worst-case ping-pong IPC under forced single-core contention

    Without pinning:

        Best-effort IPC under scheduler optimization
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/resource.h>

static void pin_to_cpu_core(int cpu_core);
static void create_pipe(int pipe[2]);
static void create_child_process(int* pid_out);
static void parse_args(int argc, char** argv, int* cpu, unsigned long* bytes);

#define CPU_FLAG "--cpu"
#define BYTES_FLAG "--bytes"
#define DEFAULT_BYTES_TO_SEND 123456

int main(int argc, char** argv)
{
    int cpu = -1;
    unsigned long bytes_to_send = DEFAULT_BYTES_TO_SEND;
    
    parse_args(argc, argv, &cpu, &bytes_to_send);
    
    if (cpu >= 0) {
        pin_to_cpu_core(cpu);
        printf("Pinning to cpu core: %d\n", cpu);
    } else {
        printf("Not pinning to a particular cpu core\n");
    }
    
    printf("Bytes to send: %ld\n", bytes_to_send);
    
    struct timeval start, end;
    struct rusage ru_before, ru_after;
    int a_to_b[2], b_to_a[2];
    int pid_a, pid_b;
    
    create_pipe(a_to_b);
    create_pipe(b_to_a);

    create_child_process(&pid_a);
    
    if (pid_a == 0) {
        char recv;
        char data = 'A'; 
        unsigned long i = 0;

        close(a_to_b[0]);
        close(b_to_a[1]);

        if (write(a_to_b[1], &data, 1) != 1)
            _exit(1);
            
        while (read(b_to_a[0], &recv, 1) > 0) {            
            if (write(a_to_b[1], &data, 1) != 1)
                _exit(1);

            if (i++ >= bytes_to_send)
                break;            
        }
        
        close(a_to_b[1]);
        close(b_to_a[0]);
        _exit(0);
    }

    create_child_process(&pid_b);

    if (pid_b == 0) {
        char recv;
        char data = 'A';
        unsigned long i = 0;

        close(a_to_b[1]);
        close(b_to_a[0]);

        while (read(a_to_b[0], &recv, 1) > 0) {                        
            if (write(b_to_a[1], &data, 1) != 1)
                _exit(1);

            if (i++ >= bytes_to_send)
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

    gettimeofday(&start, NULL);
    getrusage(RUSAGE_CHILDREN, &ru_before);

    waitpid(pid_a, NULL, 0);
    waitpid(pid_b, NULL, 0);

    getrusage(RUSAGE_CHILDREN, &ru_after);
    gettimeofday(&end, NULL);
    time_t sec = end.tv_sec - start.tv_sec;
    suseconds_t usec = end.tv_usec - start.tv_usec;

    if (usec < 0) {
        sec--;
        usec += 1000000;
    }

    printf("Elapsed: %ld.%06ld seconds\n", sec, usec);
    printf(
        "Voluntary Context Switches: %ld\nInvoluntary Context Switches: %ld\n", 
        ru_after.ru_nvcsw - ru_before.ru_nvcsw,
        ru_after.ru_nivcsw - ru_before.ru_nivcsw
    );
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

static void parse_args(int argc, char** argv, int* cpu, unsigned long* bytes)
{
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc) {
            if (strcmp(argv[i], CPU_FLAG) == 0) {
                long nproc = sysconf(_SC_NPROCESSORS_ONLN);
                *cpu = atoi(argv[i + 1]);

                if (*cpu >= nproc) {
                    printf("cpu cores: %ld\n", nproc);
                    printf("--cpu: invalid indexs %d\n", *cpu);
                    _exit(1);
                }
                
            }
            
            if (strcmp(argv[i], BYTES_FLAG) == 0) {
                const char* str = argv[i + 1];
                if (str[0] == '-') {
                    printf("--bytes: Negative values not allowed\n");
                    _exit(1);
                }

                char* endptr;
                errno = 0;
                unsigned long val = strtoul(argv[i + 1], &endptr, 10);

                if (errno == ERANGE) {
                    printf("--bytes: Value out of range.\n");
                    _exit(1);
                } else if (endptr == str) {
                    printf("--bytes: No digits found.\n");
                    _exit(1);
                }

                *bytes = val;
            }
        }
    }
}
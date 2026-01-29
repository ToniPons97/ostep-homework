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

static void pin_to_cpu_core(int cpu_core);
static void create_pipe(int pipe[2]);
static void create_child_process(int* pid_out);
static void parse_args(int argc, char** argv, int* cpu, size_t* bytes);

#define CPU_FLAG "--cpu"
#define BYTES_FLAG "--bytes"
#define DEFAULT_BYTES_TO_READ 100000

int main(int argc, char** argv)
{
    int cpu = -1;
    size_t bytes_to_send = -1;
    
    parse_args(argc, argv, &cpu, &bytes_to_send);
    
    if (cpu >= 0) {
        pin_to_cpu_core(cpu);
        printf("Pinning to cpu core: %d\n", cpu);
    } else {
        printf("Not pinning to a particular cpu core\n");
    }
    
    if (bytes_to_send < 0)
        bytes_to_send = DEFAULT_BYTES_TO_READ;
    
    printf("Bytes to send: %ld\n", bytes_to_send);
    
    struct timeval start, end;
    int a_to_b[2], b_to_a[2];
    int pid_a, pid_b;
    
    create_pipe(a_to_b);
    create_pipe(b_to_a);

    gettimeofday(&start, NULL);


    create_child_process(&pid_a);
    
    if (pid_a == 0) {
        char recv;
        char data = 'A'; 
        size_t i = 0;

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
        size_t i = 0;

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

    waitpid(pid_a, NULL, 0);
    waitpid(pid_b, NULL, 0);

    gettimeofday(&end, NULL);
    time_t sec = end.tv_sec - start.tv_sec;
    suseconds_t usec = end.tv_usec - start.tv_usec;

    if (usec < 0) {
        sec--;
        usec += 1000000;
    }

    printf("Elapsed: %ld.%06ld seconds\n", sec, usec);
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

static void parse_args(int argc, char** argv, int* cpu, size_t* bytes)
{
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc) {
            if ((strncmp(argv[i], CPU_FLAG, strlen(CPU_FLAG)) == 0)) {
                long nproc = sysconf(_SC_NPROCESSORS_ONLN);
                *cpu = atoi(argv[i + 1]);

                if (*cpu >= nproc) {
                    printf("cpu cores: %ld\n", nproc);
                    printf("--cpu: invalid index: %d\n", *cpu);
                    _exit(1);
                }
                
            }
            
            if ((strncmp(argv[i], BYTES_FLAG, strlen(BYTES_FLAG)) == 0) && (i + 1 < argc)) {
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
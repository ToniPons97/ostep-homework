#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <float.h>

typedef struct {
    uint64_t count;
    double mean;
    double m2;
    double min;
    double max;
    int last_cpu;
    uint64_t n_cpu_migrations;
} stats_t;

static inline void pin_to_cpu_core(int cpu_core);
static inline void count_cpu_migration(stats_t* s);
static inline void update_stats(stats_t *s, double value);
double get_mean(stats_t *s);
double get_variance(stats_t *s);
double get_stddev(stats_t *s);
int parse_args(int argc, char** argv);

int main(int argc, char** argv)
{
    if (parse_args(argc, argv) != 0)
        return 1;

    struct timeval time = {0, 0};
    struct timezone* tz = NULL;
    size_t measurements = 0;
    size_t n_measurements = 50;
    time_t old_secs = 0, curr_secs = 0;
    suseconds_t old_usecs = 0, curr_usecs = 0;
    stats_t stats = {
        .count = 0,
        .mean = 0,
        .m2 = 0,
        .min = DBL_MAX,
        .max = -DBL_MAX,
        .last_cpu = sched_getcpu(),
        .n_cpu_migrations = 0
    };
    int n_syscalls = 10000;

    int res = gettimeofday(&time, tz);
    if (res == -1) {
        perror("gettimeofday");
        exit(1);
    }
    // sleep(1);

    old_secs = time.tv_sec;
    old_usecs = time.tv_usec;

    while(measurements++ < n_measurements) {
        count_cpu_migration(&stats);

        int res = gettimeofday(&time, tz);
        if (res == -1) {
            perror("gettimeofday");
            exit(1);
        }

        curr_secs = time.tv_sec;
        curr_usecs = time.tv_usec;
        time_t sec_diff = curr_secs - old_secs;
        suseconds_t usec_diff = curr_usecs - old_usecs;
        suseconds_t total_usec = sec_diff * 1000000L + usec_diff;


        // printf("Measurement: %ld\nSeconds: %ld\nMicroseconds: %ld\nDifference (sec): %ld\nDifference (usec): %ld\n", 
        //     measurements,
        //     curr_secs, 
        //     curr_usecs,
        //     curr_secs - old_secs,
        //     usec_diff
        // );

        for (int i = 0; i < n_syscalls; i++)
            getpid();

        update_stats(&stats, (double)total_usec);

        old_secs = curr_secs;
        old_usecs = curr_usecs;
    }

    printf("\n============================ STATS ===========================\n");
    printf("\nMin: %f\nMax: %f\nCPU Migrations: %ld\nMean: %f\nVariance: %f\nStd Dev: %f\n", 
        stats.min,
        stats.max,
        stats.n_cpu_migrations,
        get_mean(&stats),
        get_variance(&stats),
        get_stddev(&stats)
    );

    return 0;
}

static inline void pin_to_cpu_core(int cpu_core) 
{
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu_core, &set);
    
    if (sched_setaffinity(0, sizeof(set), &set)) {
        perror("sched_setaffinity");
        exit(1);
    }
}

static inline void log_cpu(void) 
{
    printf("\n[cpu core: %d]\n", sched_getcpu());
}

static inline void count_cpu_migration(stats_t* s)
{
    int curr_cpu = sched_getcpu();
    if (curr_cpu != s->last_cpu) {
        s->n_cpu_migrations++;
        s->last_cpu = curr_cpu;
    }

    // printf("\n[cpu core: %d]\n", curr_cpu);
}

static inline void update_stats(stats_t *s, double x)
{
    s->count++;

    double delta = x - s->mean;
    s->mean += delta / s->count;
    double delta2 = x - s->mean;
    s->m2 += delta * delta2;

    if (x < s->min) s->min = x;
    if (x > s->max) s->max = x;
}


double get_mean(stats_t *s)
{
    return s->mean;
}

double get_variance(stats_t *s)
{
    return s->count > 1 ? s->m2 / s->count : 0.0;
}

double get_stddev(stats_t *s)
{
    return sqrt(get_variance(s));
}


int parse_args(int argc, char** argv) 
{
    if (argc == 3) {
        if (strncmp(argv[1], "--pin-cpu", strlen("--pin-cpu")) != 0) {
            printf("invalid flag... aborting...\n");
            return 1;
        }
    
        int cpu = atoi(argv[2]);
        if (cpu < 0) {
            printf("cpu core can't be negative... aborting...\n");
            return 1;
        }

        pin_to_cpu_core(cpu);
    }

    return 0;
}
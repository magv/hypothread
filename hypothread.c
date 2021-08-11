/* Hypothread executes the command it was given as arguments, but
 * sets its CPU affinity such that hyperthreading is avoided.
 *
 * Usage:
 *
 *     hypothread command [argument] ...
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

int
parse_proc_cpuinfo(int maxcpus, int *physical_id, int *core_id)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f == NULL) {
        return 1;
    }
    int this_processor = -1;
    int this_core_id = -1;
    int this_physical_id = -1;
    int max_processor = -1;
    char *line = NULL;
    size_t size = 0;
    for (int i = 0; i < maxcpus; i++) {
        physical_id[i] = -1;
        core_id[i] = -1;
    }
    for (;;) {
        ssize_t n = getline(&line, &size, f);
        if (n <= 0) break;
        if (memcmp(line, "processor", 9) == 0) {
            char *s = line + 9;
            while ((*s == ':') || isspace(*s)) s++;
            this_processor = atoi(s);
            continue;
        }
        if (memcmp(line, "physical id", 11) == 0) {
            char *s = line + 11;
            while ((*s == ':') || isspace(*s)) s++;
            this_physical_id = atoi(s);
            continue;
        }
        if (memcmp(line, "core id", 7) == 0) {
            char *s = line + 7;
            while ((*s == ':') || isspace(*s)) s++;
            this_core_id = atoi(s);
            continue;
        }
        if (strcmp(line, "\n") == 0) {
            if ((0 <= this_processor) && (this_processor < maxcpus)) {
                if (this_processor > max_processor) max_processor = this_processor;
                physical_id[this_processor] = this_physical_id;
                core_id[this_processor] = this_core_id;
            }
            this_processor = -1;
            this_core_id = -1;
            this_physical_id = -1;
        }
    }
    if ((0 <= this_processor) && (this_processor < maxcpus)) {
        if (this_processor > max_processor) max_processor = this_processor;
        physical_id[this_processor] = this_physical_id;
        core_id[this_processor] = this_core_id;
    }
    if (line != NULL) free(line);
    fclose(f);
    return max_processor;
}

int
main(int argc, char *argv[])
{
    if (argc <= 1) {
        fprintf(stderr, "usage: hypothread command [argument] ...\n");
        exit(1);
    }
    if (get_nprocs_conf() != get_nprocs()) {
        fprintf(stderr, "hypothread: only %d cpus are available out of %d configured\n", get_nprocs(), get_nprocs_conf());
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);
    int r = sched_getaffinity(0, sizeof(mask), &mask);
    if (r != 0) {
        printf("sched_getaffinity() failed with code %d: %s\n", errno, strerror(errno));
        exit(1);
    }
    fprintf(stderr, "hypothread: affinity set to %d cpus out of %d\n", CPU_COUNT(&mask), get_nprocs());
    int *physical_id = calloc(CPU_SETSIZE, sizeof(int));
    int *core_id = calloc(CPU_SETSIZE, sizeof(int));
    int maxprocessor = parse_proc_cpuinfo(CPU_SETSIZE, physical_id, core_id);
    if (maxprocessor < 0) {
        fprintf(stderr, "hypothread: can't open /proc/cpuinfo");
        exit(1);
    };
    for (int i = 0; i <= maxprocessor; i++) {
        if (!CPU_ISSET(i, &mask)) continue;
        // O(n^2) search... eh.
        bool repeat = false;
        for (int j = i - 1; j >= 0; j--) {
            if (!CPU_ISSET(j, &mask)) continue;
            if ((physical_id[i] == physical_id[j]) && (core_id[i] == core_id[j])) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            fprintf(stderr, "hypothread: cpu %d, physical id %d, core id %d => will skip\n", i, physical_id[i], core_id[i]);
            CPU_CLR(i, &mask);
        } else {
            fprintf(stderr, "hypothread: cpu %d, physical id %d, core id %d\n", i, physical_id[i], core_id[i]);
        }
    }
    free(physical_id);
    free(core_id);
    fprintf(stderr, "hypothread: will use %d cpus\n", CPU_COUNT(&mask));
    r = sched_setaffinity(0, sizeof(mask), &mask);
    if (r != 0) {
        printf("sched_setaffinity() failed with code %d: %s\n", errno, strerror(errno));
        exit(1);
    }
    fprintf(stderr, "hypothread: will run");
    for (int i = 1; i < argc; i++) {
        fprintf(stderr, " [%s]", argv[i]);
    }
    fprintf(stderr, "\n");
    execvp(argv[1], &argv[1]);
    fprintf(stderr, "hypothread: failed to run '%s': %s\n", argv[1], strerror(errno));
    return 1;
}

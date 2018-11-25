#include <stdint.h>
#include "../include/libavutil/rational.h"
#include <sched.h>

int sched_getaffinity(pid_t pid, void* cpusetsize, void *mask) {
    return -1;
}
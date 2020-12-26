#include <sched.h>
#include <unistd.h>
#include <stdio.h>

#include "types.h"

#define MIN_CPU_COUNT 0ULL
#define MAX_CPU_COUNT 0xffffffffffffffffULL

int g_cpu_nums = 0;

/*inline int get_cpuid(void)
{
    // todo
    return -1;
    // return sched_getcpu();
}*/

u64 current_cpu = 0;
int get_cpuid(void)
{
    u64 cpuid;

    __sync_bool_compare_and_swap(&current_cpu, MAX_CPU_COUNT, MIN_CPU_COUNT);
    cpuid = __sync_fetch_and_add(&current_cpu, 1);
    
    return cpuid % g_cpu_nums;
}


int get_cpu_nums(void)
{
    return sysconf(_SC_NPROCESSORS_CONF);
}


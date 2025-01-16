#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const long long MEM_FORMAT_THRESHOLD = 8LL << 30;

#if defined(__APPLE__) && defined(__MACH__)

#include <mach/mach.h>
#include <sys/sysctl.h>

void get_cpu_stat(long long *tot, long long *idle) {
    host_cpu_load_info_data_t stat;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    int ret = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&stat, &count);
    assert(ret == KERN_SUCCESS);
    *idle = stat.cpu_ticks[CPU_STATE_IDLE];
    *tot = 0;
    for (int i = 0; i < CPU_STATE_MAX; i++) *tot += stat.cpu_ticks[i];
}

void get_mem_usage(long long *mem_used, long long *mem_total) {
    int ret;
    vm_size_t page_size;
    ret = host_page_size(mach_host_self(), &page_size);
    assert(ret == KERN_SUCCESS);
    vm_statistics64_data_t stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    ret = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&stat, &count);
    assert(ret == KERN_SUCCESS);
    *mem_used = (stat.internal_page_count - stat.purgeable_count + stat.wire_count + stat.compressor_page_count) * page_size;
    size_t tot_size = sizeof(*mem_total);
    ret = sysctlbyname("hw.memsize", mem_total, &tot_size, NULL, 0);
    assert(ret == 0);
}

#else

void get_cpu_stat(long long *tot, long long *idle) {
    FILE *fp = fopen("/proc/stat", "r");
    assert(fp != NULL);
    long long user, nice, system;
    int ret = fscanf(fp, "cpu %lld %lld %lld %lld", &user, &nice, &system, idle);
    assert(ret == 4);
    *tot = user + nice + system + *idle;
}

void get_mem_usage(long long *mem_used, long long *mem_total) {
    FILE *fp = fopen("/proc/meminfo", "r");
    assert(fp != NULL);
    long long mem_free, mem_available;
    int ret = fscanf(fp, "MemTotal: %lld kB\nMemFree: %lld kB\n", mem_total, &mem_free);
    assert(ret == 2);
    *mem_total <<= 10;
    mem_free <<= 10;
    if (fscanf(fp, "MemAvailable: %lld kB\n", &mem_available) == 1) {
        mem_available <<= 10;
        *mem_used = *mem_total - mem_available;
    } else {
        *mem_used = *mem_total - mem_free;
    }
}

#endif

double get_cpu_usage(useconds_t sample_duration) {
    long long tot1, idle1, tot2, idle2;
    get_cpu_stat(&tot1, &idle1);
    usleep(sample_duration);
    get_cpu_stat(&tot2, &idle2);

    while (tot1 == tot2) {
        usleep(sample_duration); // XXX: lazy hack
        get_cpu_stat(&tot2, &idle2);
    }

    return 1 - 1.0 * (idle2 - idle1) / (tot2 - tot1);
}

int main(int argc, char *argv[]) {
    useconds_t sample_duration;
    if (argc >= 2) {
        sample_duration = strtod(argv[1], NULL) * 1e6;
    } else {
        sample_duration = 100000;
    }
    double cpu_usage = get_cpu_usage(sample_duration);
    long long mem_used, mem_total;
    get_mem_usage(&mem_used, &mem_total);
    if (mem_total <= MEM_FORMAT_THRESHOLD) {
        printf("%4.0f%% %lld/%lldM\n", cpu_usage * 100, mem_used >> 20, mem_total >> 20);
    } else {
        printf("%4.0f%% %.1f/%.1fG\n", cpu_usage * 100, 1.0 * mem_used / (1 << 30), 1.0 * mem_total / (1 << 30));
    }
    return 0;
}

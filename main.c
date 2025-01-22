#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Cpu stats in ticks.
typedef struct cpu_stat {
  int64_t idle;
  int64_t total;
} cpu_stat;

// Memory stats in bytes.
typedef struct {
  int64_t used;
  int64_t total;
} mem_stat;

const long long MEM_FORMAT_THRESHOLD = 8LL << 30;

#if defined(__APPLE__) && defined(__MACH__)

#include <mach/mach.h>
#include <sys/sysctl.h>

cpu_stat get_cpu_stat(void) {
  host_cpu_load_info_data_t stat;
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
  int ret = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                            (host_info_t)&stat, &count);
  assert(ret == KERN_SUCCESS);
  int64_t total = 0;
  for (int i = 0; i < CPU_STATE_MAX; i++) total += stat.cpu_ticks[i];
  return (cpu_stat){.idle = stat.cpu_ticks[CPU_STATE_IDLE], .total = total};
}

mem_stat get_mem_usage(void) {
  vm_size_t page_size;
  int ret = host_page_size(mach_host_self(), &page_size);
  assert(ret == KERN_SUCCESS);
  vm_statistics64_data_t stat;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  ret = host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&stat, &count);
  assert(ret == KERN_SUCCESS);
  int64_t used = (int64_t)(stat.internal_page_count - stat.purgeable_count +
                           stat.wire_count + stat.compressor_page_count) *
                 page_size;
  int64_t total = 0;
  size_t tot_size = sizeof(total);
  ret = sysctlbyname("hw.memsize", &total, &tot_size, NULL, 0);
  assert(ret == 0);
  return (mem_stat){.used = used, .total = total};
}

#else

cpu_stat get_cpu_stat(void) {
  FILE *fp = fopen("/proc/stat", "r");
  assert(fp != NULL);
  long long user = 0, nice = 0, system = 0, idle = 0;
  int ret = fscanf(fp, "cpu %lld %lld %lld %lld", &user, &nice, &system, &idle);
  assert(ret == 4);
  return (cpu_stat){.idle = idle, .total = user + nice + system + idle};
}

mem_stat get_mem_usage(void) {
  FILE *fp = fopen("/proc/meminfo", "r");
  assert(fp != NULL);
  long long mem_total = 0, mem_unused = 0;
  int ret =
      fscanf(fp, "MemTotal: %lld kB\nMemFree: %lld kB\nMemAvailable: %lld kB\n",
             &mem_total, &mem_unused, &mem_unused);
  // May not have MemAvailable on older kernels.
  assert(ret >= 2);
  return (mem_stat){.used = (mem_total - mem_unused) << 10,
                    .total = mem_total << 10};
}

#endif

double get_cpu_usage(useconds_t sample_duration) {
  cpu_stat s0 = get_cpu_stat(), s1 = {};
  do {
    usleep(sample_duration);  // XXX: lazy hack
    s1 = get_cpu_stat();
  } while (s1.total == s0.total);
  return 1 - 1.0 * (s1.idle - s0.idle) / (s1.total - s0.total);
}

int main(int argc, char *argv[]) {
  useconds_t sample_duration;
  if (argc >= 2) {
    sample_duration = strtod(argv[1], NULL) * 1e6;
  } else {
    sample_duration = 100000;
  }
  double cpu_usage = get_cpu_usage(sample_duration);
  mem_stat mem = get_mem_usage();
  if (mem.total <= MEM_FORMAT_THRESHOLD) {
    printf("%4.0f%% %lld/%lldM\n", cpu_usage * 100, mem.used >> 20,
           mem.total >> 20);
  } else {
    printf("%4.0f%% %.1f/%.1fG\n", cpu_usage * 100, 1.0 * mem.used / (1 << 30),
           1.0 * mem.total / (1 << 30));
  }
  return 0;
}

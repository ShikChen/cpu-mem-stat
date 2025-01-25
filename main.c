#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CHECK(expr)                                                    \
  if (!(expr)) {                                                       \
    fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #expr); \
    exit(1);                                                           \
  }

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

void sleep_ms(int duration_ms) {
  // usleep() is deprecated and clock_nanosleep() is not available on macOS.
  struct timespec ts = {.tv_sec = duration_ms / 1000,
                        .tv_nsec = (duration_ms % 1000) * 1000000};
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

#if defined(__APPLE__) && defined(__MACH__)

#include <mach/mach.h>
#include <sys/sysctl.h>

cpu_stat get_cpu_stat(void) {
  // See
  // https://github.com/apple-oss-distributions/xnu/blob/8d741a5de7ff4191bf97d57b9f54c2f6d4a15585/osfmk/kern/host.c#L1162
  // for the implementation of host_processor_info().
  //
  // Note that we don't use host_statistics() with HOST_CPU_LOAD_INFO because
  // its update frquency is too low.
  natural_t cpu_count = 0;
  processor_info_array_t info = NULL;
  mach_msg_type_number_t info_count = 0;
  int ret = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO,
                                &cpu_count, &info, &info_count);
  CHECK(ret == KERN_SUCCESS);
  int64_t idle = 0, total = 0;
  for (natural_t i = 0; i < cpu_count; i++) {
    unsigned int *ticks = ((processor_cpu_load_info_data_t *)info)[i].cpu_ticks;
    idle += ticks[CPU_STATE_IDLE];
    for (int j = 0; j < CPU_STATE_MAX; j++) {
      total += ticks[j];
    }
  }
  ret = vm_deallocate(mach_task_self(), (vm_address_t)info,
                      info_count * sizeof(*info));
  CHECK(ret == KERN_SUCCESS);
  return (cpu_stat){.idle = idle, .total = total};
}

mem_stat get_mem_usage(void) {
  vm_size_t page_size;
  int ret = host_page_size(mach_host_self(), &page_size);
  CHECK(ret == KERN_SUCCESS);
  vm_statistics64_data_t stat;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  ret = host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&stat, &count);
  CHECK(ret == KERN_SUCCESS);
  int64_t used = (int64_t)(stat.internal_page_count - stat.purgeable_count +
                           stat.wire_count + stat.compressor_page_count) *
                 page_size;
  int64_t total = 0;
  size_t tot_size = sizeof(total);
  ret = sysctlbyname("hw.memsize", &total, &tot_size, NULL, 0);
  CHECK(ret == 0);
  return (mem_stat){.used = used, .total = total};
}

#else

cpu_stat get_cpu_stat(void) {
  FILE *fp = fopen("/proc/stat", "r");
  CHECK(fp != NULL);
  long long user = 0, nice = 0, system = 0, idle = 0;
  int ret = fscanf(fp, "cpu %lld %lld %lld %lld", &user, &nice, &system, &idle);
  CHECK(ret == 4);
  return (cpu_stat){.idle = idle, .total = user + nice + system + idle};
}

mem_stat get_mem_usage(void) {
  FILE *fp = fopen("/proc/meminfo", "r");
  CHECK(fp != NULL);
  long long mem_total = 0, mem_unused = 0;
  int ret =
      fscanf(fp, "MemTotal: %lld kB\nMemFree: %lld kB\nMemAvailable: %lld kB\n",
             &mem_total, &mem_unused, &mem_unused);
  // May not have MemAvailable on older kernels.
  CHECK(ret >= 2);
  return (mem_stat){.used = (mem_total - mem_unused) << 10,
                    .total = mem_total << 10};
}

#endif

// Get CPU usage percentage over a sample duration.
int get_cpu_usage(int sample_duration_ms) {
  cpu_stat s0 = get_cpu_stat();
  sleep_ms(sample_duration_ms);
  cpu_stat s1 = get_cpu_stat();
  int64_t total = s1.total - s0.total;
  int64_t busy = total - (s1.idle - s0.idle);
  // The number of ticks can be non-increasing due to integer wrap-around or too
  // short sample duration.
  if (total <= 0 || busy < 0 || busy > total) return 0;
  return (100 * busy + total / 2) / total;
}

int main(int argc, char *argv[]) {
  int sample_duration_ms = argc >= 2 ? atoi(argv[1]) : 100;
  int cpu = get_cpu_usage(sample_duration_ms);
  mem_stat mem = get_mem_usage();
  if (mem.total <= MEM_FORMAT_THRESHOLD) {
    printf("%3d%% %" PRId64 "/%" PRId64 "M\n", cpu, mem.used >> 20,
           mem.total >> 20);
  } else {
    printf("%3d%% %.1f/%.1fG\n", cpu, 1.0 * mem.used / (1 << 30),
           1.0 * mem.total / (1 << 30));
  }
  return 0;
}

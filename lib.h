#include <inttypes.h>
#include <stddef.h>

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

cpu_stat get_cpu_stat(void);
mem_stat get_mem_stat(void);

// Get CPU usage percentage over a sample duration.
int sample_cpu_usage(int duration_ms);

void format_stats(int cpu, mem_stat mem, char *buf, size_t size);

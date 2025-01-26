#include <stdio.h>
#include <stdlib.h>

#include "lib.h"

int main(int argc, char *argv[]) {
  int sample_duration_ms = argc >= 2 ? atoi(argv[1]) : 100;
  int cpu = sample_cpu_usage(sample_duration_ms);
  mem_stat mem = get_mem_stat();
  char buf[128] = {};
  format_stats(cpu, mem, buf, sizeof(buf));
  puts(buf);
  return 0;
}

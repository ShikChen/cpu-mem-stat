#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib.h"

const int DEFAULT_DURATION_MS = 100;

// TODO: Consider making the output format customizable.
typedef struct {
  int duration_ms;
} stat_opts;

static void print_help(const char *progname) {
  printf("Usage: %s [options]\n\n", progname);
  printf("Options:\n");
  printf("  -d, --duration <ms>  Sample duration in milliseconds");
  printf(" (default: %d)\n", DEFAULT_DURATION_MS);
  printf("  -h, --help           Display this help and exit\n");
}

static stat_opts parse_args(int argc, char *argv[]) {
  stat_opts opts = {.duration_ms = DEFAULT_DURATION_MS};

  struct option long_opts[] = {
      {"duration", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0},
  };

  // TODO: Change NULL to nullptr after we have C23 compdb.
  int opt = 0;
  while ((opt = getopt_long(argc, argv, "d:h", long_opts, NULL)) != -1) {
    switch (opt) {
      case 'd':
        opts.duration_ms = atoi(optarg);
        break;
      case 'h':
        print_help(argv[0]);
        exit(0);
      default:
        print_help(argv[0]);
        exit(1);
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
    print_help(argv[0]);
    exit(1);
  }
  return opts;
}

int main(int argc, char *argv[]) {
  stat_opts opts = parse_args(argc, argv);
  int cpu = sample_cpu_usage(opts.duration_ms);
  mem_stat mem = get_mem_stat();
  char buf[128] = {};
  format_stats(cpu, mem, buf, sizeof(buf));
  puts(buf);
  return 0;
}

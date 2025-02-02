#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib.h"

#ifndef VERSION  // Defined by the build system in release.
#define VERSION "0.0.0"
#endif

static_assert("" VERSION "", "VERSION must be a string literal");

const int DEFAULT_DURATION_MS = 200;

// TODO: Consider making the output format customizable.
typedef struct {
  int duration_ms;
} stat_opts;

static void print_help(const char *prog) {
  printf("Usage: %s [options]\n\n", prog);
  printf("Options:\n");
  printf("  -d, --duration <ms>  Sample duration in milliseconds");
  printf(" (default: %d)\n", DEFAULT_DURATION_MS);
  printf("  -h, --help           Display this help and exit\n");
  printf("  -v, --version        Print version\n");
}

static stat_opts parse_args(int argc, char *argv[]) {
  stat_opts opts = {.duration_ms = DEFAULT_DURATION_MS};

  struct option long_opts[] = {
      {"duration", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
      {0, 0, 0, 0},
  };

  int opt = 0;
  while ((opt = getopt_long(argc, argv, "d:h", long_opts, nullptr)) != -1) {
    switch (opt) {
      case 'd':
        opts.duration_ms = atoi(optarg);
        break;
      case 'h':
        print_help(argv[0]);
        exit(0);
      case 'v':
        printf("cpu-mem-stat %s\n", VERSION);
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

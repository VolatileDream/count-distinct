#include "libhyperloglog.h"
#include "app.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "third-party/smhasher/src/MurmurHash2.h"

void usage(char *arg0) {
  printf("usage: %s\n\n", arg0);
  printf("\nCreate a new counter, not compatible with -l\n");
  printf("  --precision|-p # : sets the counter precision (default 6).\n");
  printf("                     Larger values consume more memory.\n");
  printf("  --seed|-S $      : sets the hash seed.\n");
  printf("\nLoad and restore counters:\n");
  printf("  --load|-l <file> : tries to load the specified counter.\n");
  printf("                     Can be specified any number of times.\n");
  printf("                     Attempting to load incompatible counters\n");
  printf("                     will cause all but the first to be ignored.\n");
  printf("  --save|-s <file> : saves the counter upon exiting the program.\n");
  printf("                     (assuming no error occured)\n");
  printf("                     Can only be set once.\n");
  printf("\n  --help|-h|-? : prints this usage message\n");
}

int run(app_t *app, int argc, char* argv[]) {
  struct option options[] = {
    // { char* name, has_arg, int*flag, int }, // short name
    // File io options
    { "save", required_argument, 0, 's' }, // s:
    { "load", required_argument, 0, 'l' }, // l:

    // Counter setup options
    { "precision", required_argument, 0, 'p' }, // p:
    { "seed", required_argument, 0, 'S' }, // S:

    // Funny options
    { "help", no_argument, 0, 'h' }, // h

    { 0, 0, 0, 0 },
  };

  char* precision = 0;
  char* seed = 0;

  while(true) {
    const int c = getopt_long(argc, argv, "s:l:p:S:h", options, 0);
    if (c == -1) {
      break;
    }

    switch(c) {
     case 's':
      if (!app_queue_save(app, optarg)) {
        fprintf(stderr, "--save|-s specified more than once!");
        return 2;
      }
      break;
     case 'l':
      if (app_load(app, optarg) != 0) {
        fprintf(stderr, "unable to load file: %s\n", optarg);
        return 3;
      }
      break;
     case 'p':
      if (precision) {
        fprintf(stderr, "false positive rate already set: %s, can't use %s\n", precision, optarg);
        return 5;
      }
      precision = optarg;
      break;
     case 'S':
      if (seed) {
        fprintf(stderr, "hash seed already set: %s, can't use %s\n", seed, optarg);
        return 6;
      }
      seed = optarg;
      break;
     case '?':
     case 'h':
      usage(argv[0]);
      return 1;
    }
  }

  // Attempt to create new filter.
  if (precision || seed) {
    if(!app_create_counter(app, precision, seed)) {
      fprintf(stderr, "error creating filter: was a filter already loaded?\n");
    }
  }

  app_count(app, stdin, stdout);

  if(!app_maybe_save_on_exit(app)) {
    fprintf(stderr, "error saving filter\n");
    return 8;
  }

  return 0;
}
int main(int argc, char **argv) {
  hash_func hf =
  #if __INTPTR_WIDTH__ == 32
    &MurmurHash64B;
  #elif __INTPTR_WIDTH__ == 64
    &MurmurHash64A;
  #else
    #error Unknown pointer width.
  #endif
  app_t *app = app_init(hf);
  int rc = run(app, argc, argv);
  app_del(app);
  return rc;
}

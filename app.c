#include "app.h"

#include <stdlib.h>
#include <string.h>

#define MAX_KEY_LEN 8096

struct count_app {
  hash_func func;
  count_t *counter;
  char *save;
};

// Require opening a file.
FILE* req_open(const char *file, const char *mode) {
  FILE *f = fopen(file, mode);
  if (!f) {
    char buffer[1024] = {0};
    sprintf(buffer, "Error occurred with open(%s,%s): ", file, mode);
    perror(buffer);
    exit(1);
  }
  return f;
}

app_t* app_init(hash_func func) {
  app_t *res = (app_t*) malloc(sizeof(app_t));
  res->counter = 0;
  res->save = 0;
  res->func = func;
  return res;
}
void app_del(app_t* a) {
  if (a->counter) {
    hll_del(a->counter);
  }
  free(a);
}

bool app_create_counter(app_t *a, char *precision, char *seed) {
  if (a->counter) {
    return false;
  }
  int p = 15;
  if (precision) {
    p = atoi(precision);
  }
  int s = 0;
  if (seed) {
    s = atoi(seed);
  }
  a->counter = hll_init(a->func, p, s);

  return true;
}

int app_load(app_t *a, char *file) {
  FILE *input = req_open(file, "r");
  count_t *fl = hll_read_from_file(input, a->func);
  fclose(input);
  if (!fl) {
    return 1;
  }

  if (!a->counter) {
    a->counter = fl;
    return 0;
  }

  count_t *next = hll_merge(a->counter, fl);
  if (!next) {
    return 2;
  }

  hll_del(a->counter);
  hll_del(fl);
  a->counter = next;
  return 0;
}
bool app_queue_save(app_t *a, char *file) {
  if (a->save) {
    return false;
  }
  a->save = file;
  return true;
}
bool app_maybe_save_on_exit(app_t *a) {
  if (!a->save) {
    return true;
  }
  FILE *output = req_open(a->save, "w");
  bool res = hll_write_to_file(a->counter, output);
  fclose(output);
  return res;
}

bool read_key(FILE *in, char *buf, uint32_t length, uint32_t *read) {
  uint32_t len = 0;
  while (len < length) {
    int c = fgetc(in);
    if (c == EOF) {
      return false;
    } else if (c == '\n') {
      break;
    }
    buf[len] = (char)c;
    len++;
  }

  *read = len;
  if (len >= length) {
    // consume the rest of the line.
    int c = 0;
    while (c != '\n' && c != EOF) {
      c = fgetc(in);
    }
  }
  return true;
}

void app_count(app_t *a, FILE *in, FILE *out) {
  if (!a->counter) {
    app_create_counter(*a, NULL, NULL);
  }

  char buffer[MAX_KEY_LEN + 1] = {0};
  uint32_t length = 0;

  while (read_key(in, buffer, MAX_KEY_LEN, &length)) {
    hll_add(a->counter, buffer, length);
  }

  fprintf(out, "%lu\n", hll_count(a->counter));
}


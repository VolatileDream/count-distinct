#ifndef _APP_H_
#define _APP_H_

#include "libhyperloglog.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct count_app app_t;
struct count_app;

app_t* app_init(hash_func func);
void app_del(app_t* a);

bool app_create_counter(app_t *a, char *precision, char *seed);

// Returns error code on failure, zero on success.
int app_load(app_t *a, char *file);
bool app_queue_save(app_t *a, char *file);
bool app_maybe_save_on_exit(app_t *a);

void app_count(app_t *a, FILE *in, FILE *out);

#endif /* _APP_H_ */

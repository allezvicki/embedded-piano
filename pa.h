#ifndef _PA_H
#define _PA_H

#include <stdint.h>

#define NUM_STREAM 20
#define DEFAULT_LATENTY 300000

typedef struct {
    const char *client_name;
    const char *device_name;
} pa_run_args;

void* (pa_run)(void *args);
int pa_play(const uint8_t *buf, uint32_t  len);

#endif

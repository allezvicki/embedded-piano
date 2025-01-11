#ifndef _AUDIO_H
#define _AUDIO_H

#include <sndfile.h>
#include "keyboard.h"

#define NUM_NOTES 88
#define NUM_OCTAVES 8

typedef struct {
    uint8_t *buf;
    uint32_t len;
} notebuf;

int audio_playnote(const key *k);
int audio_init(const char *pa_device, const char *client);

#endif

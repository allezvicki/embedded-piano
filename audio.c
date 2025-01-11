#include "audio.h"

#include <malloc.h>

#include "pa.h"

static notebuf note_bufs[NUM_NOTES];

static char *note_names[] = {
    "c", "cs", "d", "ds", "e", "f", "fs", "g", "gs", "a", "as", "b",
};

static uint8_t *read_audiofile(const char *path, uint32_t *len) {
    SF_INFO sf_info;
    SNDFILE *sndfile;
    short *buf;
    sf_info.format = 0;

    if((sndfile = sf_open(path, SFM_READ, &sf_info)) == NULL) {
        fprintf(stderr, "sf_open() failed: %s\n", sf_strerror(NULL));
        return NULL;
    }
    if ((sf_info.format & SF_FORMAT_PCM_16) == 0) {
        fprintf(stderr, "wrong format, only supports 16 bit signed PCM data\n");
        return NULL;
    }
    buf = malloc(sizeof(short) * sf_info.channels * sf_info.frames);
    if (sf_readf_short(sndfile, buf, sf_info.frames) != sf_info.frames) {
        fprintf(stderr, "sf_readf_short() read less than expected\n");
        return NULL;
    }
    if (sf_close(sndfile) != 0) {
        fprintf(stderr, "sf_cloase() failed\n");
    }
    *len = sf_info.frames * sf_info.channels * 2;
    return (uint8_t *)buf;
}

int audio_init(const char *pa_device, const char *client) {
    fprintf(stderr, "Initializing audio...\n");

    /* initialize PulseAudio client */
    pthread_t pa_thread;
    pa_run_args args;

    args.device_name = pa_device;
    args.client_name = client;
    
    /* run PulseAudio client */
    if (pthread_create(&pa_thread, NULL, pa_run, &args) != 0) {
        fprintf(stderr, "pthread_create() error\n");
        return -1;
    }

    /* read notes audio from wav files */
    fprintf(stderr, "Reading notes\n");
    char path[20];
    int cnt = 0;
    for(int i = 0; i < NUM_OCTAVES; i++) {
        for(int j = 0; j < 12; j++) {
            int idx = j + i * 12 - 8;
            if (idx < 0) continue;
            if (idx >= NUM_NOTES) break;
            if (note_names[j][1] == '\0') {
                // white key
                sprintf(path, "audio/%c%d.wav", note_names[j][0], i);
            } else {
                sprintf(path, "audio/%c%d%c.wav", note_names[j][0], i, note_names[j][1]);
            }
            note_bufs[idx].buf = read_audiofile(path, &note_bufs[idx].len);
            if (note_bufs[idx].buf != NULL) {
		cnt++;
		printf("read file: %s, len %d\n", path, note_bufs[idx].len);
	    }
        }
    }
    fprintf(stderr, "%d notes read\n", cnt);
    sleep(1);
    return 0;
}

static void get_note_buf(const key *key, notebuf **buf) {
    int idx = 0;
    if (key->white) {
        if(key->num < 3) idx = key->num * 2;
        else idx = key->num * 2 - 1;
    } else {
        if (key->num < 2) idx = key->num * 2 + 1;
        else idx = key->num * 2;
    }

    idx = key->octave * 12 + idx - 8;
    if (idx < 0 || idx >= NUM_NOTES) {
        *buf = NULL;
        return;
    }
    *buf = note_bufs + idx;
    return;    
}

int audio_playnote(const key *k) {
    notebuf *buf;
    get_note_buf(k, &buf);
    if (buf == NULL || buf->buf == NULL) {
        fprintf(stderr, "audio_playnote: play %s key %d-%d error",
                k->white ? "white" : "black", k->octave, k->num);
    }
    return pa_play(buf->buf, buf->len);
}

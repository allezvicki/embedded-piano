#include "pa.h"

#include <stdio.h>
#include <pulse/pulseaudio.h>

static pa_context *context = NULL;
static pa_stream *streams[NUM_STREAM];
static volatile unsigned char playing[NUM_STREAM];
static pa_mainloop_api *mainloop_api = NULL;
static char *client_name = NULL, *device_name = NULL;
static pa_volume_t volume;
static int volume_is_set = 1;
static pa_stream_flags_t flags = 0;

static pa_sample_spec sample_spec = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2
};

/* A shortcut for terminating the application */
static void quit(int ret) {
    assert(mainloop_api);
    mainloop_api->quit(mainloop_api, ret);
}


/* This is called whenever the context status changes */
static void context_state_callback(pa_context *c, void *userdata) {
    assert(c);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {
            int r;
            pa_buffer_attr buffer_attr;
            buffer_attr.tlength = DEFAULT_LATENTY;
            buffer_attr.prebuf = -1;
            buffer_attr.minreq = -1;
            buffer_attr.maxlength = -1;
            char stream_name[20];
            
            assert(c);

            fprintf(stderr, "Context ready\n");
            for (int i = 0; i < NUM_STREAM; i++) {
                pa_stream **stream = streams + i;
                sprintf(stream_name, "%s-%d", client_name, i);
                if (!(*stream = pa_stream_new(c, stream_name, &sample_spec, NULL))) {
                    fprintf(stderr, "pa_stream_new() failed: %s\n", pa_strerror(pa_context_errno(c)));
                    goto fail;
                }

                pa_cvolume cv;
		volume = PA_VOLUME_UI_MAX;
                if ((r = pa_stream_connect_playback(*stream, device_name, &buffer_attr, flags,
                                volume_is_set ? pa_cvolume_set(&cv, sample_spec.channels, volume) : NULL,
                                NULL)) < 0) {
                    fprintf(stderr, "pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(c)));
                    goto fail;
                }
            }
            break;
        }

        case PA_CONTEXT_TERMINATED:
            quit(0);
            break;

        case PA_CONTEXT_FAILED:
        default:
            fprintf(stderr, "Connection failure: %s\n", pa_strerror(pa_context_errno(c)));
            goto fail;
    }

    return;

fail:
    quit(1);

}

void* (pa_run)(void *_args) {
    int ret = 0;
    pa_mainloop* m = NULL;
    const pa_run_args *args = (const pa_run_args *)_args;

    client_name = pa_xstrdup(args->client_name);
    device_name = pa_xstrdup(args->device_name);

    /* Set up a new main loop */
    if (!(m = pa_mainloop_new())) {
        fprintf(stderr, "pa_mainloop_new() failed.\n");
        goto quit;
    }
    fprintf(stderr, "Mainloop setup done\n");
    mainloop_api = pa_mainloop_get_api(m);

    /* Create a new connection context */
    if (!(context = pa_context_new(mainloop_api, client_name))) {
        fprintf(stderr, "pa_context_new() failed.\n");
        goto quit;
    }
    fprintf(stderr, "Context created\n");

    pa_context_set_state_callback(context, context_state_callback, NULL);

    /* Connect the context */
    if (pa_context_connect(context, NULL, 0, NULL) < 0) {
        fprintf(stderr, "pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(context)));
        goto quit;
    }

    /* Run the main loop */
    fprintf(stderr, "Running mainloop\n");
    if (pa_mainloop_run(m, &ret) < 0) {
        fprintf(stderr, "pa_mainloop_run() failed.\n");
        goto quit;
    }

quit:
    for (int i = 0; i < NUM_STREAM; i++) {
        if(streams[i]) {
            pa_stream_unref(streams[i]);
        }
    }

    if (context)
        pa_context_unref(context);

    if (m) {
        pa_mainloop_free(m);
    }

    pa_xfree(device_name);
    pa_xfree(client_name);

    return NULL;
}

static void buffer_written_callback(void *data) {
    // do nothing
    return;
}

static void stream_drain_complete(pa_stream* s, int success, void *userdata) {
    if (!success) {
        fprintf(stderr, "Failed to drain stream: %s\n", pa_strerror(pa_context_errno(context)));
        quit(1);
    }
    for (int i = 0; i < NUM_STREAM; i++) {
        if (s == streams[i]) {
            playing[i] = 0;
            fprintf(stderr, "Stream number %d drained\n", i);
            break;
        }
    }
    return;
}

/* 
 * NOT SAFE when threaded
 * assume len always fits in buffer_attr.maxlength 
 */
int pa_play(const uint8_t *buf, uint32_t  len) {
    fprintf(stderr, "playing note! buf=%lx, len=%d\n", buf, len);
    if (buf == NULL) {
        fprintf(stderr, "pa_play: buf is NULL\n");
        return -1;
    }
    // find a free stream
    pa_stream *stream = NULL;
    int stream_num;
    int ret;
    for (int i = 0; i < NUM_STREAM; i++) {
        if (!playing[i]) {
            stream_num = i;
            stream = streams[i];
            break;
        }
    }
    if (stream == NULL) {
        fprintf(stderr, "pa_play: all streams are playing\n");
        return -1;
    }
    fprintf(stderr, "stream %d is free\n", stream_num);
    if(pa_stream_get_state(stream) != PA_STREAM_READY) {
        fprintf(stderr, "pa_play: stream not ready\n");
        return -1;
    }
    /* write buf to stream */
    ret = pa_stream_write(stream, buf, len, buffer_written_callback, 0, PA_SEEK_RELATIVE);
    if(ret != 0) {
        fprintf(stderr, "pa_stream_write() failed: %s\n", pa_strerror(pa_context_errno(context)));
        quit(1);
        return -1;
    }
    playing[stream_num] = 1;
    /* TODO: I wonder if buffer_written_callback and stream_drain_callback are actually the same thing here? */
    pa_stream_drain(stream, stream_drain_complete, NULL);
    return 0;
}

#include "common/common.h"
#include "keyboard.h"
#include "touch.h"
#include "audio.h"

int main() {
    if (audio_init("AudioCodec-Playback", "piano") != 0) {
        fprintf(stderr, "audio_init() error\n");
        return 1;
    }
	fb_init("/dev/fb0");
    init_keyboard();
    init_touch_event("/dev/input/event2");
    task_loop();
}

#include "touch.h"

#include "common/common.h"
#include "audio.h"


struct last_position {
	int x, y;
} last_positions[FINGER_NUM_MAX];


static key pressed_keys[FINGER_NUM_MAX];
static unsigned char pressing[FINGER_NUM_MAX];
static unsigned char started_moving = 0;

static void touch_event_cb(int fd) {
	int type, x, y, finger;
	type = touch_read(fd, &x, &y, &finger);
	if (type == TOUCH_ERROR) {
		printf("close touch fd\n");
		close(fd);
		task_delete_file(fd);
	}
	struct last_position *last = last_positions + finger;
    key *pressed_key = pressed_keys + finger;
    key new_pressed_key;
	switch (type) {
		case TOUCH_PRESS:
            printf("EVENT TOUCH_PRESS, x=%d, y=%d\n", x, y);
            if (!started_moving && !get_key(x, y, pressed_key)) {
                // KEY PRESSED
                press_key(pressed_key);
                audio_playnote(pressed_key);
                pressing[finger] = 1;
            } else if (y >= PIANO_Y + PIANO_H) {
                // START MOVING KEYBOARD
                started_moving = 1;
            }
            last->x = x;
            last->y = y;
			break;
		case TOUCH_MOVE:
            printf("EVENT TOUCH_MOVE, x=%d, y=%d\n", x, y);
            if (started_moving) {
                move_keyboard(x - last->x);
            } else if (!get_key(x, y, &new_pressed_key)) {
                if(new_pressed_key.white == pressed_key->white
                        && new_pressed_key.num == pressed_key->num
                        && new_pressed_key.octave == pressed_key->octave) {
                    // MOVED, BUT PRESSING SAME KEY
                    ;
                } else {
                    // PRESSED NEW KEY
                    release_key(pressed_key);
                    press_key(&new_pressed_key);
                    // TODO: when same key is pressed again, should stop playing the former note
                    audio_playnote(pressed_key);
                    pressed_key->octave = new_pressed_key.octave;
                    pressed_key->num = new_pressed_key.num;
                    pressed_key->white = new_pressed_key.white;
                    pressed_key->left = new_pressed_key.left;
                }
            }
            last->x = x;
            last->y = y;
			break;
		case TOUCH_RELEASE:
            printf("EVENT TOUCH_RELEASE, x=%d, y=%d\n", x, y);
            if (pressing[finger]) {
                pressing[finger] = 0;
                release_key(pressed_key);
            } else if (started_moving) {
                started_moving = 0;
                move_keyboard(x - last->x);
            }
			break;
		default:
			return;
	}
	fb_update();
	return;
}

void init_touch_event(char* file) {
	int touch_fd = touch_init(file);
	task_add_file(touch_fd, touch_event_cb);
}

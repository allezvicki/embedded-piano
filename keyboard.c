#include "keyboard.h"

#include <stdio.h>
#include <assert.h>

#include "common/common.h"

fb_image *full_piano;
fb_image *small_piano;
/* subimage of full_piano, to fit the screen*/
fb_image *pressed_white[7];
fb_image *pressed_black;

static int x_offset = DEFAULT_XOFFSET;
static int bright_length = SMALL_PIANO_W * PIANO_W / FULL_PIANO_W;

static int BLACK_KEY_OFFSETS[] = {49, 145, 286, 377, 468};

/* small piano down below indicates current range of keys */
void draw_indicator() {
    fb_draw_image(SMALL_PIANO_X, SMALL_PIANO_Y, small_piano, 0);
    int left_length = SMALL_PIANO_W * x_offset / FULL_PIANO_W;
    int right_length = SMALL_PIANO_W - left_length - bright_length;
    fb_draw_rect(SMALL_PIANO_X, SMALL_PIANO_Y, left_length, SMALL_PIANO_H, SHADOW);
    fb_draw_rect(SMALL_PIANO_X + left_length + bright_length, SMALL_PIANO_Y, right_length, SMALL_PIANO_H, SHADOW);
}

void init_keyboard() {
    /* load images */
    printf("loading images...\n");
    full_piano = fb_read_png_image("images/piano88.png");
    assert(full_piano != NULL);
    small_piano = fb_read_png_image("images/piano88-small.png");
    assert(small_piano != NULL);
    fb_image *piano = fb_get_sub_image(full_piano, x_offset, 0, PIANO_W, PIANO_H);
    assert(piano != NULL);

    char pressed_key_image[25];
    for(int i = 0; i < 7; i++) {
        sprintf(pressed_key_image, "images/press-white%d.png", i + 1);
        pressed_white[i] = fb_read_png_image(pressed_key_image);
        assert(pressed_white[i] != NULL);
    }

    pressed_black = fb_read_png_image("images/press-black.png");
    assert(pressed_black != NULL);

    /* background */
    printf("drawing background...\n");
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    /* piano */
    printf("drawing piano...\n");
    fb_draw_image(PIANO_X, PIANO_Y, piano, 0);
    fb_free_image(piano);
    /* indicator */
    printf("drawing indicator...\n");
    draw_indicator();
    fb_update();
}

/* move keyboard by dis */
void move_keyboard(int dis) {
    /* calculate x_offset */
    dis = dis * FULL_PIANO_W / SMALL_PIANO_W;
    if (!dis) return;
    x_offset += dis;
    if (x_offset > XOFFSET_MAX) {
        x_offset = XOFFSET_MAX;
    } else if (x_offset < 0) {
        x_offset = 0;
    }

    /* draw piano */
    fb_image *piano = fb_get_sub_image(full_piano, x_offset, 0, PIANO_W, PIANO_H);
    fb_draw_image(PIANO_X, PIANO_Y, piano, 0);
    fb_free_image(piano);

    /* draw indicator */
    draw_indicator();
}

void press_key(const key *k) {
    /* draw shadow */
    printf("PRESSING KEY left: %d, white: %d, octave: %d, num: %d\n", k->left, k->white, k->octave, k->num);
    fb_image *img = k->white ? pressed_white[k->num] : pressed_black;
    fb_draw_image(k->left - x_offset, PIANO_Y, img, 0);

    /* play note */
}

void release_key(const key *k) {
    /* remove pressing shadow */
    printf("RELEASING KEY left: %d, white: %d, octave: %d, num: %d\n", k->left, k->white, k->octave, k->num);
    fb_image *key_image;
    if (k->white) {
        key_image = fb_get_sub_image(full_piano, k->left, 0, WKEY_W, WKEY_H);
    } else {
        key_image = fb_get_sub_image(full_piano, k->left, 0, BKEY_W, BKEY_H);
    }
    fb_draw_image(k->left - x_offset, PIANO_Y, key_image, 0);
    fb_free_image(key_image);

    /* stop playing note */
}

/* returns 0 if is key, -1 otherwise */
int get_key(int x, int y, key *k) {
    if (y < PIANO_Y || y >= PIANO_Y + PIANO_H) return -1;

    k->octave = (x + x_offset + 5 * WKEY_W) / OCTAVE_W;
    int within_octave = (x + x_offset + 5 * WKEY_W) % OCTAVE_W;
    unsigned char is_white = 1;

    if (y <= BKEY_H) {
        // COULD BE BLACK
        for (int i = 0; i < 5; i++) {
            int sub = within_octave - BLACK_KEY_OFFSETS[i];
            if(sub >= 0 && sub < BKEY_W) {
                k->num = i;
                k->left = k->octave * OCTAVE_W + BLACK_KEY_OFFSETS[i] - 5 * WKEY_W;
                is_white = 0;
                break;
            }
        }
    }

    k->white = is_white;
    if (is_white) {
        k->num = within_octave / WKEY_W;
        k->left = k->octave * OCTAVE_W + k->num * WKEY_W - 5 * WKEY_W;
    }
    return 0;
}

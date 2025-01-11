#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "common/common.h"

#define WKEY_W 80
#define WKEY_H 480
#define BKEY_W 46
#define BKEY_H 280
#define FULL_PIANO_W 4160 
#define FULL_PIANO_H WKEY_H
#define PIANO_W 1024 
#define PIANO_H WKEY_H 
#define SMALL_PIANO_W 1024
#define SMALL_PIANO_H 61
#define LOWER_MARGIN_H 3

#define OCTAVE_W (WKEY_W * 7)

#define PIANO_X 0
#define PIANO_Y 56

#define SMALL_PIANO_X 0
#define SMALL_PIANO_Y (PIANO_Y + PIANO_H + LOWER_MARGIN_H)

#define DEFAULT_XOFFSET 2000
#define XOFFSET_MAX (FULL_PIANO_W - PIANO_W)

#define BACKGROUND_COLOR BLACK
#define SHADOW 0x80000000

typedef struct {
    unsigned char white;
    unsigned char octave;
    unsigned char num;
    int left;
} key;


void init_keyboard();
void draw_indicator();
void move_keyboard(int dis);
void press_key(const key *k);
void release_key(const key *k);
int get_key(int x, int y, key *k);



#endif

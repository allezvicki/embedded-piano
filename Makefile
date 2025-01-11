CC := gcc

CFLAGS :=-Wall -Ofast -fopenmp -fopt-info-vec-optimized -I/usr/include/freetype2/
DEBUGCFLAGS := -Wall -fopenmp -fopt-info-vec-optimized -g -O0 -I/usr/include/freetype2/

LDFLAGS := -fopenmp
LDLIBS := -ljpeg -lfreetype -lpng -lc -lpulse -lsndfile

SRC := common/graphic.c common/touch.c common/image.c common/task.c keyboard.c touch.c pa.c audio.c main.c

TARGET := piano

OBJ := $(patsubst %.c, %.o, $(SRC))

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

clean:
	rm -f $(TARGET) $(OBJ)

%.o: %.c common/common.h
	$(CC) $(DEBUGCFLAGS) -c -o $@ $<


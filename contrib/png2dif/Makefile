TARGET = png2dif
LIBS =
CC = gcc
CFLAGS = -g -Wall -I./ -I ../../include -I ../../third_party/upng/include -I ../../third_party/adafruit/include

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
SRCS    = ../../third_party/upng/src/upng.c
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) $(SRCS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)


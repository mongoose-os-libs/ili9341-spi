TARGET = png2dif
LIBS =
CC = gcc
CFLAGS = -g -Wall -I./ -I ../../include

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
SRCS    = 
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) $(SRCS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)


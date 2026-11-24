
CC = gcc

CFLAGS = -Wall -Wextra -O2 -std=gnu11 \
         $(shell pkg-config --cflags gtk4)

LIBS = $(shell pkg-config --libs gtk4) \
       -lgpiod -lpthread -lm

TARGET = robotest

SOURCES = robotest.c \
          balance_loop.c \
          gui.c \
          imu.c \
          bno055.c \
          encoder.c \
          pwm.c

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

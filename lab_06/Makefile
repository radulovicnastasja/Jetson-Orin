
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11 -pthread
LDLIBS = -lgpiod -lpthread -lm

TARGET = lab06
OBJS = main.o gamepad.o encoder.o pwm.o speed_control.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

TARGET = main

CC = gcc
CFLAGS = -Wall -Wextra -O3
LIBS = -lgpiod -lpthread -lm

SRCS = main.c encoder.c pwm.c speed_control.c
OBJS = $(SRCS:.c=.o)

.RECIPEPREFIX := >

all: $(TARGET)

$(TARGET): $(OBJS)
>$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
>$(CC) $(CFLAGS) -c $< -o $@

clean:
>rm -f $(OBJS) $(TARGET)

.PHONY: all clean
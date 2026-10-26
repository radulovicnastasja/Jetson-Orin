
#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <stdbool.h>

typedef struct {
    int fd;

    int vertical;
    int horizontal;

    bool fast;
    bool stopped;
    bool running;
    bool armed;
    bool brake_held;

} gamepad_t;

int gamepad_open(gamepad_t *pad, const char *device);
int gamepad_poll(gamepad_t *pad);
void gamepad_close(gamepad_t *pad);

#endif

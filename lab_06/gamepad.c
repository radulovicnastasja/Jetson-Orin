
#include "gamepad.h"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

int gamepad_open(gamepad_t *pad, const char *device)
{
    memset(pad, 0, sizeof(*pad));

    pad->fd = -1;
    pad->running = true;
    pad->stopped = true;
    pad->armed = false;
    pad->brake_held = false;

    pad->fd = open(device, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

    if (pad->fd < 0) {
        perror(device);
        return -1;
    }

    struct stat st;

    if (fstat(pad->fd, &st) || !S_ISCHR(st.st_mode)) {
        fprintf(stderr,
                "Not an input character device: %s\n",
                device);
        gamepad_close(pad);
        return -1;
    }

    printf("Gamepad open: %s\n", device);
    printf("Starting STOPPED. Release D-pad before driving.\n");

    return 0;
}

int gamepad_poll(gamepad_t *pad)
{
    struct input_event e;

    for (;;) {
        ssize_t n = read(pad->fd, &e, sizeof(e));

        if (n == 0) {
            fprintf(stderr, "Gamepad disconnected\n");
            return -1;
        }

        if (n < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;

            perror("gamepad read");
            return -1;
        }

        if (n != sizeof(e)) {
            fprintf(stderr, "Incomplete gamepad event\n");
            return -1;
        }

        // Stop if events were lost.
        if (e.type == EV_SYN && e.code == SYN_DROPPED) {
            fprintf(stderr,
                    "Gamepad event overflow; stopping\n");
            return -1;
        }

        // -------------------------
        // D-PAD CONTROLS
        // -------------------------
        if (e.type == EV_ABS) {

            // UP / DOWN
            if (e.code == ABS_HAT0Y) {

                int old = pad->vertical;

                if (e.value < 0)
                    pad->vertical = 1;   // Forward
                else if (e.value > 0)
                    pad->vertical = -1;  // Reverse
                else
                    pad->vertical = 0;   // Released

                // A released direction rearms the controller,
                // provided the emergency stop is not held.
                if (pad->vertical == 0 &&
                    !pad->brake_held) {
                    pad->armed = true;
                }

                // A fresh direction press may restart movement.
                if (old == 0 &&
                    pad->vertical != 0 &&
                    pad->armed &&
                    !pad->brake_held) {

                    pad->stopped = false;
                }
            }

            // LEFT / RIGHT
            else if (e.code == ABS_HAT0X) {

                if (e.value < 0)
                    pad->horizontal = -1;
                else if (e.value > 0)
                    pad->horizontal = 1;
                else
                    pad->horizontal = 0;
            }
        }

        // -------------------------
        // BUTTON PRESSED
        // -------------------------
        else if (e.type == EV_KEY && e.value == 1) {

            // X: Toggle slow / fast
            if (e.code == BTN_X) {

                pad->fast = !pad->fast;

                printf("Mode: %s\n",
                       pad->fast ? "FAST" : "SLOW");
            }

            // B: Emergency stop
            else if (e.code == BTN_EAST) {

                pad->stopped = true;
                pad->armed = false;
                pad->brake_held = true;

                puts("STOP: Release B and D-pad before restarting");
            }

            // START: Exit program
            else if (e.code == BTN_START) {

                pad->stopped = true;
                pad->running = false;

                puts("START pressed: Exiting");
            }
        }

        // -------------------------
        // BUTTON RELEASED
        // -------------------------
        else if (e.type == EV_KEY && e.value == 0) {

            // B released
            if (e.code == BTN_EAST) {

                pad->brake_held = false;

                // Only rearm if the direction is released.
                if (pad->vertical == 0)
                    pad->armed = true;

                puts("B released");
            }
        }
    }
}

void gamepad_close(gamepad_t *pad)
{
    if (pad->fd >= 0)
        close(pad->fd);

    pad->fd = -1;
}

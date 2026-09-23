#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <gpiod.h>
#include "pwm.h"

// PWM chips and channels
static const int PWM_CHIP2 = 2;
static const int PWM_CHIP3 = 3;
static const int PWM0 = 0;

// 10 kHz PWM = 100,000 ns period
#define PWM_PERIOD_NS 100000

// GPIO direction line definitions (gpiochip0)
#define GPIO_CHIP "/dev/gpiochip0"
#define DIR_LINE_OFFSET  106
#define DIR_LINE_OFFSET2 113

static struct gpiod_chip *chip;
static struct gpiod_line *dir_line[2];

// Helper function to write values to sysfs files
static int write_sysfs(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        perror(path);
        return -1;
    }

    if (write(fd, value, strlen(value)) < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int pwm_init(void)
{
    char path[100];

    // --------------------------------------------------
    // Export PWM channel on pwmchip2
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d",
             PWM_CHIP2, PWM0);

    if (access(path, F_OK) != 0) {
        snprintf(path, sizeof(path),
                 "/sys/class/pwm/pwmchip%d/export",
                 PWM_CHIP2);

        if (write_sysfs(path, "0") < 0)
            return -1;

        usleep(100000);
    }

    // --------------------------------------------------
    // Export PWM channel on pwmchip3
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d",
             PWM_CHIP3, PWM0);

    if (access(path, F_OK) != 0) {
        snprintf(path, sizeof(path),
                 "/sys/class/pwm/pwmchip%d/export",
                 PWM_CHIP3);

        if (write_sysfs(path, "0") < 0)
            return -1;

        usleep(100000);
    }

    // --------------------------------------------------
    // Configure PWM periods
    // --------------------------------------------------
    char period_str[20];

    snprintf(period_str, sizeof(period_str),
             "%d", PWM_PERIOD_NS);

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/period",
             PWM_CHIP2, PWM0);

    if (write_sysfs(path, period_str) < 0)
        return -1;

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/period",
             PWM_CHIP3, PWM0);

    if (write_sysfs(path, period_str) < 0)
        return -1;

    // --------------------------------------------------
    // Set initial duty cycle to 0
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle",
             PWM_CHIP2, PWM0);

    if (write_sysfs(path, "0") < 0)
        return -1;

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle",
             PWM_CHIP3, PWM0);

    if (write_sysfs(path, "0") < 0)
        return -1;

    // --------------------------------------------------
    // Enable PWM channels
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/enable",
             PWM_CHIP2, PWM0);

    if (write_sysfs(path, "1") < 0)
        return -1;

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/enable",
             PWM_CHIP3, PWM0);

    if (write_sysfs(path, "1") < 0)
        return -1;

    // --------------------------------------------------
    // Open GPIO chip
    // --------------------------------------------------
    chip = gpiod_chip_open(GPIO_CHIP);

    if (!chip) {
        perror("gpiod_chip_open");
        return -1;
    }

    // Get direction GPIO lines
    dir_line[0] =
        gpiod_chip_get_line(chip, DIR_LINE_OFFSET);

    dir_line[1] =
        gpiod_chip_get_line(chip, DIR_LINE_OFFSET2);

    if (!dir_line[0] || !dir_line[1]) {
        fprintf(stderr,
                "Failed to get GPIO direction lines\n");
        gpiod_chip_close(chip);
        return -1;
    }

    // Request direction lines as outputs
    if (gpiod_line_request_output(
            dir_line[0], "PWM_DIR0", 0) < 0) {

        perror("gpiod_line_request_output DIR0");
        gpiod_chip_close(chip);
        return -1;
    }

    if (gpiod_line_request_output(
            dir_line[1], "PWM_DIR1", 0) < 0) {

        perror("gpiod_line_request_output DIR1");
        gpiod_line_release(dir_line[0]);
        gpiod_chip_close(chip);
        return -1;
    }

    return 0;
}


// duty cycle setter.
// duty_cycle is a float between -1.0 and 1.0
int pwm_set_duty(int channel, float duty_cycle)
{
    // Make sure channel is valid
    if (channel != 0 && channel != 1)
        return -1;

    // Clamp duty cycle to allowed range
    if (duty_cycle > 1.0)
        duty_cycle = 1.0;

    if (duty_cycle < -1.0)
        duty_cycle = -1.0;

    // --------------------------------------------------
    // Set motor direction
    // --------------------------------------------------
    int direction;

    if (duty_cycle >= 0.0)
        direction = 1;
    else
        direction = 0;

    if (gpiod_line_set_value(
            dir_line[channel], direction) < 0) {

        perror("gpiod_line_set_value");
        return -1;
    }

    // Convert negative duty cycle to positive magnitude
    if (duty_cycle < 0.0)
        duty_cycle = -duty_cycle;

    // --------------------------------------------------
    // Convert duty cycle to nanoseconds
    // --------------------------------------------------
    char duty_cycle_str[20];

    snprintf(duty_cycle_str,
             sizeof(duty_cycle_str),
             "%d",
             (int)(duty_cycle * PWM_PERIOD_NS));

    // --------------------------------------------------
    // Select correct PWM channel
    // --------------------------------------------------
    char path[100];

    if (channel == 0) {

        snprintf(path, sizeof(path),
                 "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle",
                 PWM_CHIP2, PWM0);

    } else {

        snprintf(path, sizeof(path),
                 "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle",
                 PWM_CHIP3, PWM0);
    }

    // Write duty cycle
    if (write_sysfs(path, duty_cycle_str) < 0)
        return -1;

    return 0;
}


int pwm_cleanup(void)
{
    char path[100];

    // --------------------------------------------------
    // Disable PWM channels
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/enable",
             PWM_CHIP2, PWM0);

    write_sysfs(path, "0");

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/pwm%d/enable",
             PWM_CHIP3, PWM0);

    write_sysfs(path, "0");

    // --------------------------------------------------
    // Unexport PWM channels
    // --------------------------------------------------
    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/unexport",
             PWM_CHIP2);

    write_sysfs(path, "0");

    snprintf(path, sizeof(path),
             "/sys/class/pwm/pwmchip%d/unexport",
             PWM_CHIP3);

    write_sysfs(path, "0");

    // --------------------------------------------------
    // Release GPIO direction lines
    // --------------------------------------------------
    if (dir_line[0])
        gpiod_line_release(dir_line[0]);

    if (dir_line[1])
        gpiod_line_release(dir_line[1]);

    if (chip)
        gpiod_chip_close(chip);

    return 0;
}
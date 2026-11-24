
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <math.h>

#include "imu.h"
#include "encoder.h"
#include "pwm.h"
#include "balance_loop.h"
#include "gui.h"

#define LOOP_PERIOD_NS 1000000L
#define ENCODER_COUNTS_PER_REV 700.0

atomic_bool running = ATOMIC_VAR_INIT(1);

static void *control_thread(void *arg)
{
    (void)arg;

    unsigned int loop_count = 0;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (atomic_load(&running))
    {
        /* Read sensor values */
        double pitch = get_imu_pitch();

        double rpm0 =
            encoder_get_speed(0) * 60.0 /
            ENCODER_COUNTS_PER_REV;

        double rpm1 =
            encoder_get_speed(1) * 60.0 /
            ENCODER_COUNTS_PER_REV;

        /* Update data displayed in GTK4 */
        gui_update_data(pitch, rpm0, rpm1);

        /*
         * Controller remains disabled by default.
         * Do not enable until hardware safety checks.
         */
        balance_loop_step(loop_count++);

        /* Schedule next iteration at 1 ms */
        next.tv_nsec += LOOP_PERIOD_NS;

        if (next.tv_nsec >= 1000000000L)
        {
            next.tv_sec++;
            next.tv_nsec -= 1000000000L;
        }

        clock_nanosleep(
            CLOCK_MONOTONIC,
            TIMER_ABSTIME,
            &next,
            NULL);
    }

    return NULL;
}

int main(void)
{
    pthread_t thread;

    printf("Lab 05: Self-Balancing Robot\n");

    if (imu_init() != 0)
    {
        fprintf(stderr, "IMU initialization failed\n");
        return EXIT_FAILURE;
    }

    if (encoder_init() != 0)
    {
        fprintf(stderr, "Encoder initialization failed\n");
        imu_cleanup();
        return EXIT_FAILURE;
    }

    if (pwm_init() != 0)
    {
        fprintf(stderr, "PWM initialization failed\n");
        encoder_cleanup();
        imu_cleanup();
        return EXIT_FAILURE;
    }

    balance_loop_init();

    /* Explicitly keep controller disabled */
    balance_loop_set_enabled(0);

    if (pthread_create(
            &thread, NULL,
            control_thread, NULL) != 0)
    {
        fprintf(stderr, "Thread creation failed\n");

        balance_loop_cleanup();
        pwm_cleanup();
        encoder_cleanup();
        imu_cleanup();

        return EXIT_FAILURE;
    }

    /* GTK4 runs in the main thread */
    gui_run();

    /* Shutdown after GUI closes */
    atomic_store(&running, 0);
    pthread_join(thread, NULL);

    balance_loop_cleanup();
    pwm_cleanup();
    encoder_cleanup();
    imu_cleanup();

    printf("Lab 05 shutdown complete\n");

    return EXIT_SUCCESS;
}

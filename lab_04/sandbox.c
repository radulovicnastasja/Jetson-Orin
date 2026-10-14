#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include "imu.h"
#include "encoder.h"
#include "pwm.h"


extern atomic_bool running;

int main_t()
{

    printf("Hello world\r\n");

    if(imu_init())
    {
        fprintf(stderr, "Failed to initialize IMU\n");
        exit(EXIT_FAILURE);
    }

    if(encoder_init())
    {
        fprintf(stderr, "Failed to initialize encoder\n");
        exit(EXIT_FAILURE);
    }

    if(pwm_init())
    {
        fprintf(stderr, "Failed to initialize encoder\n");
        exit(EXIT_FAILURE);
    }

    atomic_store(&running, true); // Signal other threads to stop
    
    printf("Starting the lab\r\n");

    float target_speed = 75.0;
    float k_p = -1.0 / (10.0) * 10;
    float err = 0.0;

    for (int i; i < 5000; i++)
    {
        err = target_speed - encoder_get_speed(0);
        printf("%f,%f,%f\r\n", err,encoder_get_speed(0),k_p * err);
        if(k_p * err > 0) err = 0;
        pwm_set_duty(0,1);
        usleep(100);
    }
    pwm_set_duty(0,0);
    pwm_set_duty(1,0);

    atomic_store(&running, false); // Signal other threads to stop

    pwm_cleanup();
    imu_cleanup();
    encoder_cleanup();
    return 0;
}
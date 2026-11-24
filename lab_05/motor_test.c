#include <stdio.h>
#include <unistd.h>
#include "pwm.h"

int main(void)
{
    if (pwm_init() != 0)
    {
        fprintf(stderr, "PWM initialization failed\n");
        return 1;
    }

    printf("Testing Motor 0 at 15%% duty\n");
    pwm_set_duty(0, 0.15f);
    sleep(1);
    pwm_set_duty(0, 0.0f);

    sleep(1);

    printf("Testing Motor 1 at 15%% duty\n");
    pwm_set_duty(1, 0.15f);
    sleep(1);
    pwm_set_duty(1, 0.0f);

    pwm_cleanup();

    printf("Motor test completed\n");
    return 0;
}
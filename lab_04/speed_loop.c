#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "encoder.h"
#include "pwm.h"
#include "step_test.h"

bool speed_test_active = true;
static FILE *speed_test_data = NULL;

void speed_test_init()
{
    speed_test_data = fopen("speed_test.csv", "w");
    if (speed_test_data == NULL)
    {
        perror("step_test_data.csv");
        speed_test_active = false;
        return;
    }

    speed_test_active = true;
}

#define STEP_TEST_LENGTH 1000
int incremenet_speed(long loop_count, float duty,uint8_t channel)
{
    if(!speed_test_active)
    {
        return 0;
    }

    /* writing to the file if not NULL */
    if (speed_test_data != NULL)
    {
        fprintf(speed_test_data, "%d,%.6f,%.6f\n", loop_count, duty, encoder_get_speed(channel));
        
    }
    /* writing the new speed*/
    pwm_set_duty(channel, duty); 
    return 1;
}


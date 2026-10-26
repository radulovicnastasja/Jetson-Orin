// Speed control module: PI controller with feedforward and deadband compensation.
// speed_control_init() builds the motor model from deadband.csv and step_response.csv.
 
#include <stdio.h>
#include <math.h>
 
#include "speed_control.h"
#include "pwm.h"
#include "encoder.h"
 
#define ENCODER(m) (1 - (m))
#define DT         0.001
#define TAU_CL     0.05
#define FILTER_TAU 0.005
#define STALE_MS   50
#define MAX_DUTY   0.80
#define MAX_ROWS   20000
 
static double gain = 250.0, tau = 0.1, db_pos = 0.1, db_neg = 0.1;
static double kp, ki, integ, speed_f;
static double duty_buf[MAX_ROWS], speed_buf[MAX_ROWS];
static int active_motor = -1;
 
static void load_deadband(void)
{
    FILE *fp = fopen("deadband.csv", "r");
    char line[128];
    int t;
    long pos;
    double duty, speed;
 
    if (fp == NULL)
        return;
    if (fgets(line, sizeof line, fp) != NULL) {
        db_pos = db_neg = 0.0;
        while (fscanf(fp, "%d,%lf,%lf,%ld", &t, &duty, &speed, &pos) == 4) {
            if (duty > db_pos)
                db_pos = duty;
            if (-duty > db_neg)
                db_neg = -duty;
        }
    }
    fclose(fp);
}
 
static double avg5(int j)
{
    return (speed_buf[j - 4] + speed_buf[j - 3] + speed_buf[j - 2] + speed_buf[j - 1] + speed_buf[j]) / 5.0;
}
 
static void load_step(void)
{
    FILE *fp = fopen("step_response.csv", "r");
    char line[128];
    int n = 0, t, steps = 0;
    long pos;
    double gain_sum = 0.0, tau_sum = 0.0;
 
    if (fp == NULL)
        return;
    if (fgets(line, sizeof line, fp) != NULL)
        while (n < MAX_ROWS && fscanf(fp, "%d,%lf,%lf,%ld", &t, &duty_buf[n], &speed_buf[n], &pos) == 4)
            n++;
    fclose(fp);
 
    for (int i = 1; i < n; i++) {
        if (duty_buf[i - 1] != 0.0 || duty_buf[i] == 0.0)
            continue;
        int end = i;
        while (end < n && duty_buf[end] == duty_buf[i])
            end++;
        if (end - i < 400)
            continue;
 
        double ss = 0.0;
        for (int j = end - 300; j < end; j++)
            ss += speed_buf[j];
        ss /= 300.0;
 
        double sign = (duty_buf[i] > 0) ? 1.0 : -1.0;
        double above = fabs(duty_buf[i]) - ((sign > 0) ? db_pos : db_neg);
        if (fabs(ss) < 1e-6 || above < 0.05)
            continue;
 
        int j = i + 4;
        while (j < end && avg5(j) / ss < 0.632)
            j++;
 
        gain_sum += ss / (sign * above);
        tau_sum += (j - i) / 1000.0;
        steps++;
    }
    if (steps > 0) {
        gain = gain_sum / steps;
        tau = tau_sum / steps;
    }
}
 
void speed_control_init(void)
{
    load_deadband();
    load_step();
    kp = tau / (gain * TAU_CL);
    ki = 1.0 / (gain * TAU_CL);
    integ = 0.0;
    speed_f = 0.0;
    active_motor = -1;
    printf("speed_control: deadband +%.2f/-%.2f, gain %.2f per duty, tau %.3f s\n",
           db_pos, db_neg, gain, tau);
}
 
void speed_control_reset(void)
{
    integ = 0.0;
    speed_f = 0.0;
}

double speed_control_update(double target_speed,
                            double measured_speed)
{
    // Filter the measured encoder speed.
    speed_f += DT / (FILTER_TAU + DT) *
               (measured_speed - speed_f);

    double error = target_speed - speed_f;

    // Feedforward with deadband compensation.
    double ff = target_speed / gain;
    double fade = fmin(fabs(ff) / 0.02, 1.0);

    if (ff > 0.0)
        ff += db_pos * fade;
    else if (ff < 0.0)
        ff -= db_neg * fade;

    // PI controller.
    double candidate = ff + kp * error +
                       integ + ki * error * DT;

    // Anti-windup.
    if (fabs(candidate) < MAX_DUTY)
        integ += ki * error * DT;

    double u = ff + kp * error + integ;

    // Lab 06 PWM safety limit.
    const double limit = 0.60;

    if (u > limit)
        u = limit;
    if (u < -limit)
        u = -limit;

    return u;
}

void speed_control_cleanup(void)
{
    speed_control_reset();
}

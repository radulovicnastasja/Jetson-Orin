#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include "balance_loop.h"
#include "imu.h"
#include "pwm.h"

#define DT 0.001
#define MAX_DUTY 0.75
#define MAX_PITCH_DEG 4.0
#define TARGET_PITCH_DEG -0.03
#define MAX_INTEGRAL 100.0
#define DEBUG_INTERVAL 1000

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static double kp = 0.0, ki = 0.0, kd = 0.0;
static double integral = 0.0, previous_error = 0.0;
static int first_step = 1, enabled = 0;
/* Signs assume positive PWM on motor 0 drives backward and on motor 1 forward. */
static int motor0_sign = -1, motor1_sign = 1;
static unsigned int debug_counter = 0;

static double clamp(double x, double limit)
{
    return x > limit ? limit : (x < -limit ? -limit : x);
}

static double normalized_pitch(double raw)
{
    if (!isfinite(raw)) return NAN;
    double angle = raw - 180.0;
    while (angle < -180.0) angle += 360.0;
    while (angle >= 180.0) angle -= 360.0;
    return angle;
}

/* All functions below that use PWM must hold lock. */
static void stop_locked(void)
{
    pwm_set_duty(0, 0.0f);
    pwm_set_duty(1, 0.0f);
}

static void reset_locked(void)
{
    integral = 0.0;
    previous_error = 0.0;
    first_step = 1;
}

static void disable_locked(void)
{
    enabled = 0;
    reset_locked();
    stop_locked();
}

void balance_loop_init(void)
{
    pthread_mutex_lock(&lock);
    kp = ki = kd = 0.0;
    motor0_sign = -1;
    motor1_sign = 1;
    debug_counter = 0;
    disable_locked();
    pthread_mutex_unlock(&lock);
    puts("Balance controller initialized (disabled)");
}

void balance_loop_set_gains(double p, double i, double d)
{
    if (!isfinite(p) || !isfinite(i) || !isfinite(d) || p < 0 || i < 0 || d < 0) return;
    pthread_mutex_lock(&lock);
    kp = p; ki = i; kd = d;
    reset_locked();
    printf("PID gains: Kp=%.4f Ki=%.4f Kd=%.4f\n", kp, ki, kd);
    pthread_mutex_unlock(&lock);
}

void balance_loop_get_gains(double *p, double *i, double *d)
{
    pthread_mutex_lock(&lock);
    if (p) *p = kp;
    if (i) *i = ki;
    if (d) *d = kd;
    pthread_mutex_unlock(&lock);
}

int balance_loop_is_enabled(void)
{
    pthread_mutex_lock(&lock);
    int result = enabled;
    pthread_mutex_unlock(&lock);
    return result;
}

void balance_loop_set_enabled(int value)
{
    pthread_mutex_lock(&lock);
    if (value) {
        /* Reject enable requests while tilted, before any PWM output. */
        double pitch = normalized_pitch(get_imu_pitch());
        if (!isfinite(pitch) ||
    fabs(pitch - TARGET_PITCH_DEG) > MAX_PITCH_DEG) {
            disable_locked();
            fprintf(stderr, "Enable rejected: pitch=%+.2f\n", pitch);
        } else {
            stop_locked();
            reset_locked();
            enabled = 1;
            puts("PID controller enabled");
        }
    } else {
        disable_locked();
        puts("PID controller disabled");
    }
    pthread_mutex_unlock(&lock);
}

void balance_loop_set_motor_signs(int s0, int s1)
{
    if ((s0 != 1 && s0 != -1) || (s1 != 1 && s1 != -1)) return;
    pthread_mutex_lock(&lock);
    disable_locked();
    motor0_sign = s0;
    motor1_sign = s1;
    pthread_mutex_unlock(&lock);
}

void balance_loop_step(unsigned int loop_count)
{
    (void)loop_count;
    double raw = get_imu_pitch();
    double pitch = normalized_pitch(raw);
    pthread_mutex_lock(&lock);
    if (!enabled) {
        /* PWM is already zero after disable; avoid redundant sysfs writes. */
        pthread_mutex_unlock(&lock);
        return;
    }
    /* Safety cutoff: stop both motors beyond 4 degrees */
if (!isfinite(pitch) ||
    fabs(pitch - TARGET_PITCH_DEG) >= MAX_PITCH_DEG)
{
    fprintf(stderr,
            "SAFETY CUTOFF: pitch=%+.2f degrees\n",
            pitch);

    disable_locked();

    pthread_mutex_unlock(&lock);
    return;
}
    double error = TARGET_PITCH_DEG - pitch;
    integral = clamp(integral + error * DT, MAX_INTEGRAL);
    double derivative = first_step ? 0.0 : (error - previous_error) / DT;
    first_step = 0;
    previous_error = error;
    double output = kp * error + ki * integral + kd * derivative;
    if (!isfinite(output)) {
        fputs("PID output invalid; disabling motors\n", stderr);
        disable_locked();
        pthread_mutex_unlock(&lock);
        return;
    }
    output = clamp(output, MAX_DUTY);
    double duty0 = motor0_sign * output;
    double duty1 = motor1_sign * output;
    if (++debug_counter >= DEBUG_INTERVAL) {
        printf("PID: Kp=%.4f Ki=%.4f Kd=%.4f pitch=%+.2f output=%+.3f duty0=%+.3f duty1=%+.3f\n",
               kp, ki, kd, pitch, output, duty0, duty1);
        debug_counter = 0;
    }
    /* Serialize PWM writes with GUI stop. */
    pwm_set_duty(0, (float)duty0);
    pwm_set_duty(1, (float)duty1);
    pthread_mutex_unlock(&lock);
}

void balance_loop_cleanup(void)
{
    balance_loop_set_enabled(0);
    puts("Balance controller stopped");
}

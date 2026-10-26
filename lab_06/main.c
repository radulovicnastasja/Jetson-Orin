#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>
#include <errno.h>

#include "gamepad.h"
#include "speed_control.h"
#include "encoder.h"
#include "pwm.h"

atomic_bool running = ATOMIC_VAR_INIT(true);


#define DEVICE "/dev/input/event6"
#define FEEDBACK_ENCODER 1  /* Lab04: ENCODER(0) == 1 */
#define DT_NS 1000000L      /* 1 ms controller */
#define PWM_LIMIT 0.60
#define STEER_SLOW 0.08
#define STEER_FAST 0.12
/* Lab05 observation: motor0 and motor1 need opposite electrical signs for forward. */
#define MOTOR0_FORWARD_SIGN (-1)
#define MOTOR1_FORWARD_SIGN (+1)
#define ENCODER_FORWARD_SIGN (+1)
/* Set after wheel mapping test: +1 means positive steering turns right. */
#define STEERING_SIGN (+1)
#define SLOW_SPEED_PPS 15.0
#define FAST_SPEED_PPS 40.0


static volatile sig_atomic_t quit=0;
static void on_signal(int sig) { (void)sig; quit=1; }
static double clamp(double x) { return fmax(-PWM_LIMIT,fmin(PWM_LIMIT,x)); }
static long long ms_since(struct timespec start,struct timespec now) {
    return (now.tv_sec-start.tv_sec)*1000LL+(now.tv_nsec-start.tv_nsec)/1000000LL;
}
static void add_ns(struct timespec *t,long ns) {
    t->tv_nsec+=ns;
    if (t->tv_nsec>=1000000000L) { t->tv_nsec-=1000000000L; t->tv_sec++; }
}
static void stop_motors(void) { pwm_set_duty(0,0); pwm_set_duty(1,0); }

int main(int argc,char **argv)
{
    const char *device=argc>1?argv[1]:DEVICE;
    signal(SIGINT,on_signal); signal(SIGTERM,on_signal);
    gamepad_t pad;
    if (gamepad_open(&pad,device)) return 1;
    if (pwm_init()!=0) { gamepad_close(&pad); return 1; }
    stop_motors();
    if (encoder_init()!=0) { stop_motors(); pwm_cleanup(); gamepad_close(&pad); return 1; }
    speed_control_init();
    FILE *log=fopen("lab06_log.csv","w");
    if (!log) { perror("lab06_log.csv"); stop_motors(); speed_control_cleanup(); encoder_cleanup(); pwm_cleanup(); gamepad_close(&pad); return 1; }
    fprintf(log,"time_ms,target_speed,measured_speed,left_pwm,right_pwm,mode\n");
    struct timespec start,next,now;
    clock_gettime(CLOCK_MONOTONIC,&start);
    next=start;
    long long next_log=20;
    int was_active=0, failed=0;
    puts("READY. Wheels off ground. D-pad up/down speed, left/right steer; X mode, B stop, START exit.");
    while (!quit && pad.running) {
        struct pollfd pfd={.fd=pad.fd,.events=POLLIN|POLLERR|POLLHUP|POLLNVAL};
        int pr=poll(&pfd,1,0);
        if (pr<0) { if (errno == EINTR) continue; perror("poll"); failed=1; break; }
        if (pr>0 && (pfd.revents&(POLLERR|POLLHUP|POLLNVAL))) { fputs("Gamepad disconnected\n",stderr); failed=1; break; }
        if (gamepad_poll(&pad)<0) { failed=1; break; }
        if (!pad.running) break;
        double target=(!pad.stopped && pad.vertical!=0) ? pad.vertical*(pad.fast?FAST_SPEED_PPS:SLOW_SPEED_PPS) : 0.0;
        double measured=encoder_get_speed(FEEDBACK_ENCODER);
        if (!isfinite(measured)) { fputs("Invalid encoder speed\n",stderr); failed=1; break; }
        double left=0,right=0;
        if (target!=0) {
            was_active=1;
            /* Encoder polarity is hardware-dependent; verify before motor testing. */
            double forward_measured = ENCODER_FORWARD_SIGN * measured;
            /* PI command is in forward coordinates. */
            double base=speed_control_update(target,forward_measured);
            if (!isfinite(base)) { failed=1; break; }
            double steer=STEERING_SIGN*pad.horizontal*(pad.fast?STEER_FAST:STEER_SLOW);
            double forward_left=clamp(base-steer);
            double forward_right=clamp(base+steer);
            /* Channel assignment assumption: motor0=left, motor1=right. */
            left=clamp(MOTOR0_FORWARD_SIGN*forward_left);
            right=clamp(MOTOR1_FORWARD_SIGN*forward_right);
            pwm_set_duty(0,(float)left);
            pwm_set_duty(1,(float)right);
        } else {
            stop_motors();
            if (was_active) { speed_control_reset(); was_active=0; }
        }
        clock_gettime(CLOCK_MONOTONIC,&now);
        long long elapsed=ms_since(start,now);
        if (elapsed>=next_log) {
            /* One row at each reached 20ms logging deadline; check jitter in CSV. */
            fprintf(log,"%lld,%.3f,%.3f,%.3f,%.3f,%d\n",elapsed,target,measured,left,right,pad.fast?1:0);
            next_log=((elapsed/20)+1)*20;
        }
        add_ns(&next,DT_NS);
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&next,NULL);
        clock_gettime(CLOCK_MONOTONIC,&now);
        if (ms_since(next,now)>10) next=now;
    }
    stop_motors();
    fflush(log); fclose(log);
 speed_control_cleanup();

atomic_store(&running, false);
encoder_cleanup();

pwm_cleanup();
    gamepad_close(&pad);
    puts("Motors stopped; CSV closed; modules cleaned up.");
    return failed?1:0;
}

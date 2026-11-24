#ifndef BALANCE_LOOP_H
#define BALANCE_LOOP_H
void balance_loop_init(void);
void balance_loop_step(unsigned int loop_count);
void balance_loop_set_gains(double kp, double ki, double kd);
void balance_loop_cleanup(void);
void balance_loop_set_enabled(int enabled);
void balance_loop_set_motor_signs(int motor0_sign, int motor1_sign);
int balance_loop_is_enabled(void);
void balance_loop_get_gains(double *kp, double *ki, double *kd);
#endif

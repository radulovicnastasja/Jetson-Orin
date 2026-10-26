
#ifndef SPEED_CONTROL_H
#define SPEED_CONTROL_H

void speed_control_init(void);

double speed_control_update(double target_speed,
                            double measured_speed);

void speed_control_reset(void);

void speed_control_cleanup(void);

#endif

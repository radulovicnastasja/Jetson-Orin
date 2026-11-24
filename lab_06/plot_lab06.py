#!/usr/bin/env python3
"""Plot Lab 06 CSV. Requires matplotlib and pandas."""
import sys
import pandas as pd
import matplotlib.pyplot as plt
path = sys.argv[1] if len(sys.argv) > 1 else 'lab06_log.csv'
df = pd.read_csv(path)
t = df['time_ms'] / 1000.0
fig, ax = plt.subplots(figsize=(10, 4))
ax.plot(t, df['target_speed'], label='Target (pulses/s)')
ax.plot(t, df['measured_speed'], label='Encoder (pulses/s)')
ax.set(xlabel='Time (s)', ylabel='Speed (pulses/s)', title='Lab 06 speed response')
ax.grid(True); ax.legend(); fig.tight_layout(); fig.savefig('lab06_speed.png', dpi=160)
fig, ax = plt.subplots(figsize=(10, 4))
ax.plot(t, df['left_pwm'], label='Motor 0 PWM (assumed left)')
ax.plot(t, df['right_pwm'], label='Motor 1 PWM (assumed right)')
ax.set(xlabel='Time (s)', ylabel='Signed PWM', title='Lab 06 motor commands')
ax.grid(True); ax.legend(); fig.tight_layout(); fig.savefig('lab06_pwm.png', dpi=160)
print('Saved lab06_speed.png and lab06_pwm.png')

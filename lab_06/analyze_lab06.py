import os
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

moving = pd.read_csv("lab06_moving.csv")
# Baseline source: a separate stationary recording if one exists,
# otherwise the idle periods of the moving run (target speed and both
# PWM commands zero). Idle samples count only after the motors have
# been off for SETTLE_SAMPLES in a row (25 x 20 ms = 0.5 s), so that
# coast-down after a stop is not counted as stationary noise.
SETTLE_SAMPLES = 25

if os.path.exists("lab06_baseline.csv"):
    baseline = pd.read_csv("lab06_baseline.csv")
    baseline_label = "Stationary baseline (separate run)"
else:
    idle = (
        (moving.target_speed == 0) &
        (moving.left_pwm == 0) &
        (moving.right_pwm == 0)
    )
    settled = (
        idle.astype(int).rolling(SETTLE_SAMPLES).min()
        .fillna(0).astype(bool)
    )
    baseline = moving[settled]
    baseline_label = "Stationary baseline (idle periods of moving run)"
    print(f"No lab06_baseline.csv: using {len(baseline)} settled idle "
          "samples from lab06_moving.csv as the baseline")

os.makedirs("lab06_plots", exist_ok=True)

def save_plot(name):
    plt.tight_layout()
    plt.savefig(f"lab06_plots/{name}", dpi=300)
    plt.close()

# Figure 1: Target vs measured speed
plt.figure(figsize=(11, 5))
plt.plot(moving.time_ms / 1000, moving.target_speed,
         label="Target speed", linewidth=1.5)
plt.plot(moving.time_ms / 1000, moving.measured_speed,
         label="Measured encoder speed", linewidth=1)
plt.xlabel("Time (s)")
plt.ylabel("Speed (pulses/s)")
plt.title("Lab 06: Target and Measured Speed")
plt.legend()
plt.grid(True)
save_plot("01_speed_tracking.png")

# Figure 2: Left and right PWM
plt.figure(figsize=(11, 5))
plt.plot(moving.time_ms / 1000, moving.left_pwm,
         label="Left PWM", linewidth=1)
plt.plot(moving.time_ms / 1000, moving.right_pwm,
         label="Right PWM", linewidth=1)
plt.xlabel("Time (s)")
plt.ylabel("PWM duty cycle")
plt.title("Lab 06: Left and Right Motor Commands")
plt.legend()
plt.grid(True)
save_plot("02_motor_pwm.png")

# Figure 3: Baseline vs moving
# Idle samples are not contiguous, so they are drawn as points; a line
# would join separate idle stretches across the moving periods.
plt.figure(figsize=(11, 5))
plt.plot(moving.time_ms / 1000,
         moving.measured_speed,
         label="Moving test", linewidth=1, alpha=0.7)
plt.scatter(baseline.time_ms / 1000,
            baseline.measured_speed,
            label=baseline_label, s=2, color="tab:orange", zorder=3)
plt.xlabel("Time (s)")
plt.ylabel("Measured speed (pulses/s)")
plt.title("Lab 06: Stationary vs Moving Encoder Data")
plt.legend()
plt.grid(True)
save_plot("03_baseline_comparison.png")

print("\nBaseline measured speed: mean",
      round(baseline.measured_speed.mean(), 3),
      "std", round(baseline.measured_speed.std(), 3), "pulses/s")

# Numerical analysis
active = moving[moving.target_speed != 0].copy()
active["tracking_error"] = (
    active.target_speed - active.measured_speed
)

print("\nLAB 06 DATA ANALYSIS")
print("Total moving samples:", len(moving))
print("Active motor-command samples:", len(active))
print("Modes recorded:", sorted(moving["mode"].unique()))

for mode, name in [(0, "SLOW"), (1, "FAST")]:
    subset = active[active["mode"] == mode]
    if len(subset):
        print(f"\n{name} MODE")
        print("Samples:", len(subset))
        print("Mean absolute tracking error:",
              round(subset.tracking_error.abs().mean(), 3),
              "pulses/s")
        print("RMSE:",
              round(np.sqrt(np.mean(subset.tracking_error**2)), 3),
              "pulses/s")

stationary = moving[
    (moving.target_speed == 0) &
    (moving.left_pwm == 0) &
    (moving.right_pwm == 0)
]

if len(stationary):
    print("\nStationary speed standard deviation:",
          round(stationary.measured_speed.std(), 3),
          "pulses/s")

print("\nPlots saved in lab06_plots/")
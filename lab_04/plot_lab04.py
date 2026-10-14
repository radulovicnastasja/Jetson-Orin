import pandas as pd
import matplotlib.pyplot as plt

# Plot 1: Step response
step = pd.read_csv("step_response.csv")

plt.figure(figsize=(10, 5))
plt.plot(step["time_ms"] / 1000, step["speed"],
         label="Measured Motor Speed")
plt.xlabel("Time (seconds)")
plt.ylabel("Motor Speed")
plt.title("Lab 04 - Motor Step Response")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("step_response.png", dpi=300)
plt.close()

# Plot 2: Triangular speed control
control = pd.read_csv("speed_control.csv")

plt.figure(figsize=(10, 5))
plt.plot(control["time_ms"] / 1000,
         control["target_speed"],
         label="Target Speed", linestyle="--")

plt.plot(control["time_ms"] / 1000,
         control["measured_speed"],
         label="Measured Speed")

plt.xlabel("Time (seconds)")
plt.ylabel("Motor Speed")
plt.title("Lab 04 - Three Triangular Speed Control Cycles")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("speed_control.png", dpi=300)
plt.close()

print("Plots saved successfully.")
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# Load benchmark data
df = pd.read_csv("bplus_benchmark.csv")

# Plot: Time vs Elements for different m
sns.lineplot(data=df, x="n", y="time_ms", hue="m", marker="o")
plt.title("Time vs Elements for different m")
plt.xlabel("Number of Elements (n)")
plt.ylabel("Time (ms)")
plt.grid(True)

# Overlay log(n)
# Get unique and sorted n values from the benchmark
n_unique = sorted(df["n"].unique())
logn_vals = [np.log2(n) for n in n_unique]  # or np.log(n), np.log10(n)

# Scale log(n) to match time range for visual comparison
# (min-max normalization based on time_ms)
time_min = df["time_ms"].min()
time_max = df["time_ms"].max()

logn_scaled = np.interp(logn_vals, (min(logn_vals), max(logn_vals)), (time_min, time_max))

# Plot log(n) curve
plt.plot(n_unique, logn_scaled, color="black", linestyle="--", linewidth=2, label="Scaled log₂(n)")

# Add legend
plt.legend()

# Save and show
plt.savefig("time_vs_n.png")
plt.show()
plt.clf()

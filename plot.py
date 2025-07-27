import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
df = pd.read_csv('benchmark_results_final.csv')

# Automatically get all unique page sizes in the data
all_page_sizes = sorted(df['page_size'].unique())

plt.figure(figsize=(12, 7))

max_time = 0
for page_size in all_page_sizes:
    subset = df[df['page_size'] == page_size]
    if not subset.empty:
        plt.plot(subset['num_insertions'], subset['time_ms'], label=f'Page size: {page_size//1024}KB (Rows/Page: {subset["num_insertions"].iloc[0]})')
        max_time = max(max_time, subset['time_ms'].max())

plt.xlabel('Number of Insertions')
plt.ylabel('Time (ms)')
plt.title('Time vs Number of Insertions for All Page Sizes')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.ylim(0, max_time * 1.1)
plt.savefig('benchmark_plot1.png')
plt.show()
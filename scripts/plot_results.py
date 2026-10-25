import os
import matplotlib.pyplot as plt
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUMMARY = os.path.join(ROOT, "results", "summary", "benchmark_aggregated.csv")

FRESHNESS =os.path.join(ROOT, "results", "summary", "freshness_summary.csv")

PLOT_DIR =os.path.join(ROOT, "results", "plots")

os.makedirs(PLOT_DIR, exist_ok=True)
df = pd.read_csv(SUMMARY)
freshness_df = pd.read_csv(FRESHNESS)

print("benchmark CSV loaded from:", SUMMARY)
print("Freshness CSV loaded: ", FRESHNESS)
print("Columns:", list(df.columns))

modes = ["row", "full_scalar", "full_simd", "selective_simd"]

#1
throughput_subset=df[(df["rows"] == 1000000) & (df["oltp_percent"]==50)].copy()
#subset = subset.sort_values("thread")
plt.figure()

for mode in modes:
    mode_data = throughput_subset[throughput_subset["mode"] == mode].sort_values("thread")
    plt.plot(mode_data["thread"].to_numpy(), mode_data["total_throughput_mean"].to_numpy(), marker="o", label=mode)



plt.xlabel("Worker Threads")
plt.ylabel("Mean Combined Throughput (ops/s)")
plt.title("HTAP Throughput vs Concurrency\n1M Rows, 50/50 Workload")
plt.xticks([1, 2, 4, 8])
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "throughput_vs_thread_by_mode.png"),dpi=200)
plt.close()

#2

sum_subset = df[(df["rows"] == 1000000) & (df["oltp_percent"] == 50)].copy()
#subset = subset.sort_values("thread")
plt.figure()

for mode in modes:
    mode_data = sum_subset[sum_subset["mode"] == mode].sort_values("thread")
    plt.plot(mode_data["thread"].to_numpy(), mode_data["olap_sum_p99_mean"].to_numpy(), marker="o", label=mode)


#plt.plot(subset["thread"], subset["olap_filter_p99"], marker="o", label="Price Filter")

plt.xlabel("Worker Threads")
plt.ylabel(" Mean p99 Latency (us)")
plt.title("OLAP Sum Tail Latency vs Concurrency\n1M Rows, 50/50 Workload")
plt.xticks([1, 2, 4, 8])
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "olap_sum_p99_vs_thread_by_mode.png.png"),dpi=200,)
plt.close()


#3
plt.figure()
filter_subset = df[(df["rows"] == 1000000) & (df["oltp_percent"] == 50)].copy()
plt.figure()
for mode in modes:
    mode_data = filter_subset[filter_subset["mode"] == mode].sort_values("thread")
    plt.plot(mode_data["thread"].to_numpy(), mode_data["olap_filter_p99_mean"].to_numpy(), marker="o", label=mode)

plt.xlabel("Worker Thread")
plt.ylabel("Mean p99 Latency (us)")
plt.title("OLAP Filter Tail Latency vs Concurrency\n1M Rows, 50/50 Workload")
plt.xticks([1, 2, 4, 8])
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "olap_filter_p99_vs_thread_by_mode.png"),dpi=200)

plt.close()

#4
subset_mix = df[(df["rows"]==1000000)&(df["thread"]==4)].copy()
plt.figure()
for mode in modes:
    mode_data = subset_mix[subset_mix["mode"] == mode].copy()
    mode_data = mode_data.sort_values("oltp_percent")
    plt.plot(mode_data["oltp_percent"], mode_data["total_throughput_mean"], marker="o", label=mode)
plt.xlabel("OLTP Percentage")
plt.ylabel("Mean Combined Throughput (ops/s)")
plt.title("Throughput by HTAP Workload\n1M Rows, 4 Threads")
plt.xticks([20, 50, 80])
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "throughput_by_workload_by_mode.png"),dpi=200)
plt.close()

#5
memory_subset = df[(df["rows"] == 1000000)&(df["thread"] ==4) & (df["oltp_percent"] ==50)].copy()
memory_subset["mode"] = pd.Categorical(memory_subset["mode"],categories=modes, ordered=True)
memory_subset = memory_subset.sort_values("mode")
memory_subset["total_memory_mb"] = (memory_subset["total_estimated_bytes_mean"] / (1024*1024))
plt.figure()

plt.bar(memory_subset["mode"], memory_subset["total_memory_mb"])

plt.xlabel("Benchmark Mode")
plt.ylabel("Estimated Memory (MB)")
plt.title("Estimated In-memory Storage by Mode\n1M Rows")

plt.xticks(rotation=15)
plt.grid(axis="y")
plt.tight_layout()

plt.savefig(os.path.join(PLOT_DIR, "estimated_memory_by_mode.png"),dpi=200)
plt.close()


#6
freshness_df=freshness_df.sort_values("batch_size")
plt.figure()
plt.plot(freshness_df["batch_size"], freshness_df["freshness_mean_us"], marker="o", label = "Mean Freshness")
plt.plot(freshness_df["batch_size"], freshness_df["freshness_p99_us"], marker="o", label = "p99 Freshness")
plt.xlabel("Synchronization Batch Size")
plt.ylabel("Freshness Delay (us)")
plt.title("Synchronization Freshness vs Batch Size\nSelective SIMD, 1M Rows, 4 Threads, 50/50")

plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "freshness_vs_batch_size.png"),dpi=200)
plt.close()



#subset_mix = subset_mix.sort_values("oltp_percent")

print("\nFinal Plots saved to::  ")
print(PLOT_DIR)



import os
import matplotlib.pyplot as plt
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SUMMARY = os.path.join(ROOT, "results", "summary", "benchmark_summary.csv")

PLOT_DIR =os.path.join(ROOT, "results", "plots")

os.makedirs(PLOT_DIR, exist_ok=True)
df = pd.read_csv(SUMMARY)


subset=df[(df["rows"] == 1000000) & (df["oltp_percent"]==50)]
subset = subset.sort_values("thread")
plt.figure()

plt.plot(subset["thread"], subset["total_throughput"], marker="o",)

plt.xlabel("Worker Threads")
plt.ylabel("Combined Throughput (ops/s)")
plt.title("HTAP Throughput vs Concurrency\n"
        "1M Rows, 50/50 Workload")

plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "throughput_vs_thread.png",),dpi=200)
plt.close()

plt.figure()

plt.plot(subset["thread"], subset["olap_sum_p99"], marker="o", label="SUM(price)",)
plt.plot(subset["thread"], subset["olap_filter_p99"], marker="o", label="Price Filter",)

plt.xlabel("Worker Threads")
plt.ylabel("p99 Latency (us)")
plt.title("OLAP Tail Latency vs Concurrency\n"
          "1M Rows, 50/50 Workload")

plt.legend()
plt.grid(True)

plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "olap_p99_vs_thread.png",),dpi=200,)
plt.close()

subset_mix = df[(df["rows"]==1000000)&(df["thread"]==4)]
subset_mix = subset_mix.sort_values("oltp_percent")
plt.figure()

plt.plot(subset_mix["oltp_percent"],
        subset_mix["total_throughput"],
        marker="o")

plt.xlabel("OLTP Percentage")
plt.ylabel("Combined THroughput (ops/s)")
plt.title("Throught by HTAP Workload Mix\n"
        "1M Rows, 4 Threads")

plt.grid(True)
plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "throughput_by_workload.png",),dpi=200,)

plt.close()

print("Plots sac=ved to::  ", PLOT_DIR,)



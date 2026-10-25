import os
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SUMMARY = os.path.join(ROOT, "results", "summary" , "benchmark_summary.csv",)
OUTPUT = os.path.join(ROOT, "results", "summary" , "benchmark_aggregated.csv",)

df = pd.read_csv(SUMMARY)

group_cols= ["mode", "rows", "thread", "operations_per_thread", "oltp_percent", "batch_size",]
metrics= ["total_throughput", "oltp_throughput", "olap_throughput", "oltp_read_p50", "oltp_read_p90", "oltp_read_p99", "oltp_update_p50","oltp_update_p90","oltp_update_p99", "olap_sum_p50","olap_sum_p90",  "olap_sum_p99", "olap_filter_p50","olap_filter_p90","olap_filter_p99",
        "row_store_bytes", "column_store_bytes", "total_estimated_bytes", "replica_reduction_percent",]

aggregated = (df.groupby(group_cols)[metrics].agg(["mean", "median", "std", "min", "max"]).reset_index())

aggregated.columns = ["_".join(col).strip("_")
    if isinstance(col, tuple)
    else col
    for col in aggregated.columns]

aggregated.to_csv(OUTPUT, index=False)

print("\nAggregated benchmark results is saved to: ")
print(OUTPUT)

print("\nNumber of aggregated configurations: ")
print(len(aggregated))

print("\nPreview: ")
print(aggregated[["mode", "rows", "thread", "oltp_percent", "total_throughput_mean", "total_throughput_std", "olap_sum_p99_mean","olap_filter_p99_mean" ]].sort_values(["mode", "rows", "oltp_percent", "thread"]).to_string(index=False))
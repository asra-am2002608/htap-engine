import os
import glob
import re
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RAW_DIR = os.path.join(ROOT, "results", "raw")

OUTPUT = os.path.join(ROOT, "results", "summary", "freshness_summary.csv")

pattern = os.path.join(RAW_DIR, "operations_selective_simd_1000000_4_50_*_run*.csv",)

records = []

for path in sorted(glob.glob(pattern)):
    filename = os.path.basename(path)
    match = re.search(r"_50_(\d+)_run(\d+)\.csv$", filename,)
    if not match:
        continue
    batch_size = int(match.group(1))
    run = int(match.group(2))

    if batch_size not in {1, 4, 16, 64}:
        continue

    df = pd.read_csv(path)
    df.columns = df.columns.str.strip()
    df["operation_type"] = (df["operation_type"].astype(str).str.strip())
    df["freshness_us"] = pd.to_numeric(df["freshness_us"], errors ="coerce")


    sync = df[df["operation_type"] == "SYNC"].copy()
    freshness = sync["freshness_us"].dropna()

    freshness = freshness[freshness>=0]

    if freshness.empty:
        print("warning: no freshness samples: ", filename, )
        print("operation types found:",df["operation_type"].unique())
        continue

    records.append({"batch_size" : batch_size, "run" : run, "sync_samples" : len(freshness),
        "freshness_mean_us" : freshness.mean(), "freshness_p50_us": freshness.quantile(0.50), "freshness_p90_us":freshness.quantile(0.90), "freshness_p99_us":freshness.quantile(0.99), "freshness_max_us": freshness.max()})

runs = pd.DataFrame(records)

if runs.empty:
    raise RuntimeError("No synchronization freshness data found.")

summary = (runs.groupby("batch_size").agg(runs=("run", "count"),
    sync_samples_mean=("sync_samples", "mean"),
    freshness_mean_us=("freshness_mean_us", "mean"),
    freshness_p50_us=("freshness_p50_us", "mean"),
    freshness_p90_us=("freshness_p90_us", "mean"),
    freshness_p99_us=("freshness_p99_us", "mean"),
    freshness_max_us=("freshness_max_us", "mean"),
    ).reset_index())

summary.to_csv(OUTPUT,index=False,)

print("\nPer-run freshness:")
print(runs.sort_values(["batch_size", "run"]).to_string(index=False))

print("\nAggregated freshness:")
print(summary.to_string(index=False))

print("\nSaved to: ")
print(OUTPUT)
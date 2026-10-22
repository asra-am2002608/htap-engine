#pragma once
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct OperationTrace{
    std::uint64_t timestamp_us=0;
    std::size_t thread_id=0;
    std::string operation_type;
    double latency_us = 0.0;
    std::size_t row_index =0;
    double freshness_us = 0.0;
};

struct LatencySummary{
    double p50_us = 0.0;
    double p90_us = 0.0;
    double p99_us = 0.0;

};

class MetricsCollector{
public:
    void record(const OperationTrace& trace);
    std::vector<OperationTrace> traces() const;
    LatencySummary latency_summary(const std::string& operation_type) const;
    void write_operations_csv(const std::string& path) const;
private:
    mutable std::mutex mutex_;
    std::vector<OperationTrace> traces_;

};

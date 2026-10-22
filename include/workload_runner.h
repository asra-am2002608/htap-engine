#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>

class RowStore;
class SelectiveColumnStore;
class MetricsCollector;

struct WorkloadConfig{
        std::size_t thread_count = 4;
        std:: size_t operations_per_thread = 50;

        int oltp_percent = 50;
        std::size_t sync_batch_size = 16;
        std::chrono::milliseconds sync_interval{5};
};

struct WorkloadResult{
    std::uint64_t oltp_operations = 0;
    std::uint64_t olap_operations = 0;
    std::uint64_t synchronized_changes = 0;
    double elapsed_seconds = 0.0;
    double throughput_ops_per_second =0.0;
    double oltp_throughput_ops_per_second = 0.0;
    double olap_throughput_ops_per_second = 0.0;
};

class WorkloadRunner {
public:
    WorkloadRunner(RowStore& row_store,SelectiveColumnStore& column_store, MetricsCollector& metrics);
    WorkloadResult run(const WorkloadConfig& config);


private:
    RowStore& row_store_;
    SelectiveColumnStore& column_store_;
    MetricsCollector& metrics_;
};

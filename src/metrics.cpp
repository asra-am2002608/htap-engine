#include "metrics.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

void MetricsCollector::record(const OperationTrace& trace){
    std::lock_guard lock(mutex_);
    traces_.push_back(trace);
}

std::vector<OperationTrace>MetricsCollector::traces() const{
    std::lock_guard lock(mutex_);
    return traces_;
}

LatencySummary MetricsCollector::latency_summary(const std::string& operation_type) const {
    std::vector<double> latencies;
    {std::lock_guard lock(mutex_);
    for (const auto& trace : traces_){
        if (trace.operation_type == operation_type){
            latencies.push_back(trace.latency_us);
        }
    }
    }
    LatencySummary summary;
    if (latencies.empty()){
        return summary;
    }
    std::sort(latencies.begin(), latencies.end());
    auto percentile = [&](double fraction){
        const std::size_t index = static_cast<std::size_t>(fraction * static_cast<double>(latencies.size() -1 ));
        return latencies[index];
    };

    summary.p50_us = percentile(0.50);
    summary.p90_us = percentile(0.90);
    summary.p99_us =percentile(0.99);

    return summary;
}

void MetricsCollector::write_operations_csv(const std::string& path) const{
    std::ofstream output(path);
    if(!output){
        throw std::runtime_error("Unable to open trace csv file");
    }
    output <<"timestamp_us," << "thread_id,"<< "operation_type," << "latency_us," << "row_index," << "freshness_us\n";
    std::lock_guard lock(mutex_);
    for (const auto& trace : traces_){
        output <<trace.timestamp_us << ',' << trace.thread_id << ',' << trace.operation_type << ',' << trace.latency_us << ',' << trace.row_index << ',' << trace.freshness_us << '\n';

    }

}

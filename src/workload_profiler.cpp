#include "workload_profiler.h"

WorkloadProfiler::WorkloadProfiler(std::size_t threshold) : threshold_(threshold){}
void WorkloadProfiler::record_access(ColumnId column){
    ++access_counts_[column];
}

std::size_t WorkloadProfiler::access_count(ColumnId column) const{
    const auto it = access_counts_.find(column);

    if(it ==access_counts_.end()){
        return 0;
    }
    return it->second;
}

bool WorkloadProfiler::should_materialize(ColumnId column) const{
    return access_count(column) >= threshold_;
}

std::vector<ColumnId> WorkloadProfiler::selected_columns() const{
    std::vector<ColumnId> selected;
    for (const auto& [column, count] : access_counts_){
        if (count >= threshold_){
            selected.push_back(column);
        }
    }
    return selected;
}
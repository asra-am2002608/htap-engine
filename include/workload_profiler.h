#pragma once
#include "column_id.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

class WorkloadProfiler {
public:
    explicit WorkloadProfiler(std::size_t threshold);
    void record_access(ColumnId column);
    std::size_t access_count(ColumnId column) const;
    bool should_materialize(ColumnId column) const;
    std::vector<ColumnId> selected_columns() const;

private:
std::size_t threshold_;
std::unordered_map<ColumnId, std::size_t> access_counts_;

};
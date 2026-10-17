#pragma once
#include "column_id.h"
#include <chrono>
#include <cstddef>


struct ChangeRecord
{
    std::size_t row_index;
    ColumnId column;
    double numeric_value;
    std::chrono::steady_clock::time_point commit_time;
};

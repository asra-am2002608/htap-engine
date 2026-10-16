#include <iostream>
#include <cmath>
#include <iomanip>
#include "row_store.h"
#include "data_generator.h"
#include "column_store.h"
#include "selective_column_store.h"
#include "workload_profiler.h"

int main(){
    constexpr std::size_t row_count = 100000;
    constexpr std::size_t materialization_threshold = 50;
    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore row_store(row_count);
    for(const auto& order: orders){
        row_store.insert(order);
    }

    ColumnStore full_column_store(row_count);
    full_column_store.materialize(row_store.rows());

    WorkloadProfiler profiler (materialization_threshold);
    for (int i = 0; i < 120; ++i)
    {
        profiler.record_access(ColumnId::Price);
    }
    for (int i = 0; i < 80; ++i)
    {
        profiler.record_access(ColumnId::Region);
    }
    for (int i = 0; i < 20; ++i)
    {
        profiler.record_access(ColumnId::Quantity);
    }
    for (int i = 0; i < 5; ++i)
    {
        profiler.record_access(ColumnId::Status);
    }
    const auto selected_columns = profiler.selected_columns();


    SelectiveColumnStore selective_column_store(row_count);
    selective_column_store.materialize(row_store.rows(),selected_columns);

    std::cout << "Workload access counts \n";
    std::cout << "Price: " << profiler.access_count(ColumnId::Price) << '\n';
    std::cout << "Region: " << profiler.access_count(ColumnId::Region) << '\n';
    std::cout << "Quantity: " << profiler.access_count(ColumnId::Quantity) << '\n';
    std::cout << "Status: " << profiler.access_count(ColumnId::Status) << '\n';

    std::cout << "\nMaterialaztion threshold: \n" << materialization_threshold  << '\n';
    std::cout << "\nSelected Columns\n";

    std::cout << "Price: " << (selective_column_store.has_column(ColumnId::Price) ? "YES" : "NO") << '\n';
    std::cout << "Region: " << (selective_column_store.has_column(ColumnId::Region) ? "YES" : "NO") << '\n';
    std::cout << "Quantity: " << (selective_column_store.has_column(ColumnId::Quantity) ? "YES" : "NO") << '\n';
    std::cout << "Status: " << (selective_column_store.has_column(ColumnId::Status) ? "YES" : "NO") << '\n';

    bool correct = true;
    if (!selective_column_store.has_column(ColumnId::Price)){
        correct = false;
    }
    if (!selective_column_store.has_column(ColumnId::Region)){
        correct = false;
    }
    if (selective_column_store.has_column(ColumnId::Quantity)){
        correct = false;
    }
    if (selective_column_store.has_column(ColumnId::Status)){
        correct = false;
    }


    const double row_sum = row_store.sum_price_scalar();
    const double selective_sum = selective_column_store.sum_price_scalar();
    if (std::abs(row_sum - selective_sum) > 0.0001){
        correct = false;
    }

    const std::size_t row_region_count = row_store.count_region_scalar(5);
    const std::size_t selective_region_count = selective_column_store.count_region_scalar(5);
    if (row_region_count!=selective_region_count){
        correct = false;
    }
    const std::size_t full_bytes = full_column_store.estimated_memory_bytes();
    const std::size_t selective_bytes = selective_column_store.estimated_memory_bytes();

    const double full_mb =static_cast<double>(full_bytes) / (1024.0 * 1024.0);
    const double selective_mb =static_cast<double>(selective_bytes) / (1024.0 * 1024.0);
    const double reduction = 100.0 * (1.0 -static_cast<double>(selective_bytes) / static_cast<double>(full_bytes));

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nColumn-store memory estimate\n";

    std::cout <<"Full column store : " << full_mb << "MB\n";
    std::cout <<"Workload-aware store : " << selective_mb << "MB\n";
    std::cout <<"Estimated reduction : " << reduction << "%\n";

    std::cout << "\nCorrectness chaeck: \n" << (correct ? "Passed" : "Failed")  << '\n';

    return correct ? 0:1;

}
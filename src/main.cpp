#include <iostream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include "row_store.h"
#include "data_generator.h"
#include "column_store.h"
#include "selective_column_store.h"
#include "workload_profiler.h"
#include "change_buffer.h"

int main(){
    constexpr std::size_t row_count = 100000;
    constexpr std::size_t materialization_threshold = 50;
    constexpr std::size_t sync_batch_size=4;

    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore row_store(row_count);
    for(const auto& order: orders){
        row_store.insert(order);
    }

    /*ColumnStore full_column_store(row_count);
    full_column_store.materialize(row_store.rows());*/

    WorkloadProfiler profiler (materialization_threshold);
    for (int i = 0; i < 120; ++i)
    {
        profiler.record_access(ColumnId::Price);
    }
    for (int i = 0; i < 80; ++i)
    {
        profiler.record_access(ColumnId::Region);
    }
   
    const auto selected_columns = profiler.selected_columns();


    SelectiveColumnStore selective_column_store(row_count);
    selective_column_store.materialize(row_store.rows(),selected_columns);

    ChangeBuffer change_buffer(sync_batch_size);

    constexpr std::size_t target_row = 10;
    const double original_row_price = row_store.at(target_row).price;
    const double original_column_price = selective_column_store.price_at(target_row);


    std::cout << "Before update \n";
    std::cout << "Row-store price:  " << original_row_price << '\n';
    std::cout << "Column-store price:  " << original_column_price << '\n';

    const double new_price = original_row_price+100.0;
    const auto commit_time = std::chrono::steady_clock::now();
    const std::uint64_t order_id = row_store.at(target_row).order_id;

    row_store.update_price(order_id, new_price);

    change_buffer.push(ChangeRecord{
        target_row, ColumnId::Price, new_price, commit_time

    });


    const  double stale_column_price = selective_column_store.price_at(target_row);
    std::cout << "\nAfter OLTP update, before flush \n";
    std::cout << "Row-store price:  " << row_store.at(target_row).price << '\n';
    std::cout << "Column-store price:  " << stale_column_price << '\n';

    if (change_buffer.should_flush()){
        change_buffer.flush(selective_column_store);
    }

    std::cout << "\n Buffer size after 1 update:  " << change_buffer.size() << '\n';


    const std::size_t applied = change_buffer.flush(selective_column_store);
    const auto visible_time = std::chrono::steady_clock::now();
    const auto freshness_us = std::chrono::duration_cast<std::chrono::microseconds>(
        visible_time - commit_time).count();
    const double syncronized_price = selective_column_store.price_at(target_row);

    std::cout << "\nAfteer Syncronization\n";
    std::cout << "Row-store price:  " << row_store.at(target_row).price << '\n';
    std::cout << "Column-store price:  " << syncronized_price << '\n';
    std::cout << "Changes applied: " << applied << '\n';
    std::cout << "freshness delay: " << freshness_us << "us\n";

    const bool stale_before_flush = stale_column_price != new_price;
    const bool synchronized_after_flush = syncronized_price == new_price;
    const bool correct = stale_before_flush && synchronized_after_flush &&applied ==1;
    std::cout <<"\nSynchronization correctness: " << (correct ? "Passed " : "Failed ") << '\n';

    return correct ? 0:1;


    /*std::cout << "Workload access counts \n";
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

    return correct ? 0:1;*/

}
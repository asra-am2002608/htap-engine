#include <iostream>
#include <cmath>
#include <iomanip>
#include "row_store.h"
#include "data_generator.h"
#include "column_store.h"
#include "selective_column_store.h"

int main(){
    constexpr std::size_t row_count = 100000;
    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore row_store(row_count);
    for(const auto& order: orders){
        row_store.insert(order);
    }

    ColumnStore full_column_store(row_count);
    full_column_store.materialize(row_store.rows());

    SelectiveColumnStore selective_column_store(row_count);
    selective_column_store.materialize(row_store.rows());

    const double row_sum = row_store.sum_price_scalar();
    const double full_sum=full_column_store.sum_price_scalar();
    const double selective_sum = selective_column_store.sum_price_scalar();
    const std::size_t row_region_count = row_store.count_region_scalar(5);
    const std::size_t full_region_count = full_column_store.count_region_scalar(5);
    const std::size_t selective_region_count = selective_column_store.count_region_scalar(5);
    const std::size_t row_price_count = row_store.count_price_greater_than_scalar(500.0);
    const std::size_t full_price_count = full_column_store.count_price_greater_than_scaler(500.0);
    const std::size_t selective_price_count = selective_column_store.count_price_greater_than_scalar(500.0);


    bool correct = true;

    if (std::abs(row_sum - full_sum) > 0.0001 || std::abs(row_sum - selective_sum) > 0.0001){
        correct = false;
    }
    if (row_region_count!= full_region_count || row_region_count != selective_region_count){
        correct=false;
    }
    if (row_price_count != full_price_count || row_price_count != selective_price_count){
        correct=false;
    }



    std::cout <<"Rows generated : " << row_count << '\n';
    std::cout << "\nCorrectness chaeck: pass\n" << (correct ? "Passed" : "Failed")  << '\n';


    const std::size_t full_bytes = full_column_store.estimated_memory_bytes();
    const std::size_t selective_bytes = selective_column_store.estimated_memory_bytes();

    const double full_mb =static_cast<double>(full_bytes) / (1024.0 * 1024.0);
    const double selective_mb =static_cast<double>(selective_bytes) / (1024.0 * 1024.0);

    const double reduction = 100.0 * (1.0 -static_cast<double>(selective_bytes) / static_cast<double>(full_bytes));

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nColumn-store memory estimate\n";

    std::cout <<"Full column store : " << full_mb << "MB\n";
    std::cout <<"Selective column store : " << selective_mb << "MB\n";
    std::cout <<"Estimated reduction : " << reduction << "%\n";

    if (!correct){
        return 1;
    }
    return 0;

}
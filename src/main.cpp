#include <iostream>
#include <cmath>
#include "row_store.h"
#include "data_generator.h"
#include "column_store.h"

int main(){
    constexpr std::size_t row_count = 100000;
    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore row_store(row_count);
    for(const auto& order: orders){
        row_store.insert(order);
    }

    ColumnStore column_store(row_count);
    column_store.materialize(row_store.rows());

    std::cout<<"Generated rows: " << orders.size() << '\n';
    std::cout<<"Rows store size : " << row_store.size() << '\n';
    std::cout<<"Column store size : " << column_store.size() << '\n';

    const double row_sum = row_store.sum_price_scalar();
    const double column_sum=column_store.sum_price_scalar();
    const std::size_t row_region_count = row_store.count_region_scalar(5);
    const std::size_t column_region_count = column_store.count_region_scalar(5);
    const std::size_t row_price_count = row_store.count_price_greater_than_scalar(500.0);
    const std::size_t column_price_count = column_store.count_price_greater_than_scaler(500.0);

    std::cout<<"\n Price sum\n";
    std::cout <<"Row store : " << row_sum << '\n';
    std::cout <<"Column store : " << column_sum << '\n';


    std::cout<<"\n Region 5 Count\n";
    std::cout <<"Row store : " << row_region_count << '\n';
    std::cout <<"Column store : " << column_region_count << '\n';

    std::cout<<"\n Price > 500 Count\n";
    std::cout <<"Row store : " << row_price_count << '\n';
    std::cout <<"Column store : " << column_price_count << '\n';

    bool correct = true;

    if (std::abs(row_sum - column_sum) > 0.0001){
        correct = false;
    }
    if (row_region_count!= column_region_count){
        correct=false;
    }
    if (row_price_count != column_price_count){
        correct=false;
    }

    if (correct){
        std::cout << "\nCorrectness chaeck: pass\n";
    }else{
        std::cout << "\nCorrectness chaeck: fail\n";
        return 1;
    }
    return 0;

}
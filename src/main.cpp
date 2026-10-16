#include <iostream>
#include "row_store.h"
#include "data_generator.h"

int main(){
    constexpr std::size_t row_count = 100000;
    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore store(row_count);
    for(const auto& order: orders){
        store.insert(order);
    }
    std::cout<<"Generated rows: " << orders.size() << '\n';
    std::cout<<"Rows in row store: " << store.size() << '\n';

    auto result=store.lookup(50000);

    if(result.has_value()){
        std::cout << "Ordere 50000 found \n";
        std::cout << "Price: " << result->price << '\n';
        std::cout << "Region: " << result->region << '\n';

    } else{
        std::cout <<"Order 50000 not found \n";
    } return 0;
}
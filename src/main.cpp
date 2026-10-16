#include <iostream>
#include "row_store.h"

int main(){
    RowStore store;

    Order order1{
        1,
        1001,
        501,
        1,
        2,
        199.99,
        0,
        100000
    };
    Order order2{
        2,
        1002,
        502,
        2,
        1,
        349.50,
        0,
        100001
    };
    store.insert(order1);
    store.insert(order2);

    std::cout<<"Inserted the rows: " << store.size() << '\n';
    auto result=store.lookup(2);

    if (result.has_value()){
        std:: cout << "found order 2 , the price = " << result->price << '\n';
    }
    store.update_price(2, 399.99);
    result = store.lookup(2);

    if (result.has_value()){
        std::cout << "Updated price = " << result->price << '\n';
    }
    return 0;
}
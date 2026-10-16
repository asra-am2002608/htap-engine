#include "column_store.h"

ColumnStore::ColumnStore(std::size_t reserve_size){
    if(reserve_size >0){
        order_id_.reserve(reserve_size);
        customer_id_.reserve(reserve_size);
        product_id_.reserve(reserve_size);
        region_.reserve(reserve_size);
        quantity_.reserve(reserve_size);
        price_.reserve(reserve_size);
        status_.reserve(reserve_size);
        timestamp_.reserve(reserve_size);
    }
}

void ColumnStore::materialize(const std::vector<Order>& rows){
    order_id_.clear();
    customer_id_.clear();
    product_id_.clear();
    region_.clear();
    quantity_.clear();
    price_.clear();
    status_.clear();
    timestamp_.clear();
    
    order_id_.reserve(rows.size());
    customer_id_.reserve(rows.size());
    product_id_.reserve(rows.size());
    region_.reserve(rows.size());
    quantity_.reserve(rows.size());
    price_.reserve(rows.size());
    status_.reserve(rows.size());
    timestamp_.reserve(rows.size());

    for (const auto& order : rows){
        order_id_.push_back(order.order_id);
        customer_id_.push_back(order.customer_id);
        product_id_.push_back(order.product_id);
        region_.push_back(order.region);
        quantity_.push_back(order.quantity);
        price_.push_back(order.price);
        status_.push_back(order.status);
        timestamp_.push_back(order.timestamp);
    }
}

std::size_t ColumnStore::size() const {
    return price_.size();
}

double ColumnStore::sum_price_scalar() const {
    double total = 0.0;
    for (double value : price_){
        total+=value;
    }
    return total;
}

std::size_t ColumnStore::count_region_scalar(std::uint16_t region) const{
    std::size_t count =0;
    for (std::uint16_t value : region_){
        if (value== region){
            ++count;
        }
    } return count;
}

std::size_t ColumnStore::count_price_greater_than_scaler(double threshold) const {
    std::size_t count = 0;
    for (double value : price_){
        if (value > threshold){
            ++count;
        }
    }
    return count;
}
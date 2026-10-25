#include "row_store.h"

RowStore::RowStore(std::size_t reserve_size){
    if (reserve_size > 0){
        rows_.reserve(reserve_size);
    }
}

void RowStore::insert(const Order& order){
    rows_.push_back(order);
}

std::optional<Order> RowStore::lookup(std::uint64_t order_id) const{
    for (const auto& order : rows_){
        if (order.order_id == order_id){
            return order;
        }
    }
    return std::nullopt;
}

bool RowStore::update_price(std::uint64_t order_id, double new_price){
    for (auto& order : rows_){
        if (order.order_id == order_id){
            order.price = new_price;
            return true;
        }
    }
    return false;
}

bool RowStore::update_status(std::uint64_t order_id, std::uint8_t new_status){
    for (auto& order : rows_){
        if (order.order_id == order_id){
            order.status = new_status;
            return true;
        }
    }
    return false;
}

std::size_t RowStore::size() const {
    return rows_.size();
}

const std::vector<Order>& RowStore::rows() const {
    return rows_;
}

double RowStore::sum_price_scalar() const{
    double total = 0.0;
    for (const auto& order : rows_){
        total +=order.price;
    } return total;
}

std::size_t RowStore::count_region_scalar(std::uint16_t region) const {
    std::size_t count =0;
    for (const auto& order : rows_){
        if(order.region == region){
            ++count;
        }
        } return count;
}

std::size_t RowStore::count_price_greater_than_scalar(double threshold) const{
    std::size_t count =0;
    for(const auto& order :rows_){
        if(order.price > threshold){
            ++count;
        }
    }return count;
}

const Order& RowStore::at(std::size_t row_index) const{
    return rows_.at(row_index);
}

bool RowStore::update_price_at(std::size_t row_index, double new_price){
    if(row_index >=rows_.size()){
        return false;
    }
    rows_[row_index].price=new_price;
    return true;
}

std::size_t RowStore::estimated_memory_bytes() const {
    return rows_.capacity() * sizeof(Order);
}
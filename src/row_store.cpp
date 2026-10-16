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
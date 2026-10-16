#include "selective_column_store.h"

SelectiveColumnStore::SelectiveColumnStore(std::size_t reserve_size){
    if (reserve_size > 0){
        region_.reserve(reserve_size);
        quantity_.reserve(reserve_size);
        price_.reserve(reserve_size);
    }
}

void SelectiveColumnStore::materialize(const std::vector<Order>&rows){
    region_.clear();
    quantity_.clear();
    price_.clear();
    region_.reserve(rows.size());
    quantity_.reserve(rows.size());
    price_.reserve(rows.size());

    for(const auto& order : rows){
        region_.push_back(order.region);
        quantity_.push_back(order.quantity);
        price_.push_back(order.price);
    }

}

std::size_t SelectiveColumnStore::size() const{
    return price_.size();
}
double SelectiveColumnStore::sum_price_scalar() const{
    double total = 0.0;
    for (double value:price_){
        total +=value;
    } return total;
}

std::size_t SelectiveColumnStore::count_region_scalar(std::uint16_t region) const{
    std::size_t count = 0;
    for (std::uint16_t value : region_){
        if (value == region){
            ++count;
        }
    } return count;
}

std::size_t SelectiveColumnStore::count_price_greater_than_scalar(double threshold) const{
    std::size_t count = 0;
    for (double value : price_){
        if (value > threshold){
            ++count;
        }
    } return count;
}

std::size_t SelectiveColumnStore::estimated_memory_bytes() const {
    return region_.capacity() * sizeof(std::uint16_t) + quantity_.capacity() * sizeof(std::uint32_t)+ price_.capacity()*sizeof(double);
}
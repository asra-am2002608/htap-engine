#pragma once
#include "order.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class ColumnStore {
public:
    explicit ColumnStore(std::size_t reserve_size = 0);
    void materialize(const std::vector<Order>& rows);
    std::size_t size() const;
    double sum_price_scalar() const;
    std::size_t count_region_scalar(std::uint16_t region) const;
    std::size_t count_price_greater_than_scaler(double threshold) const;

private:
std::vector<std::uint64_t> order_id_;
std::vector<std::uint64_t> customer_id_;
std::vector<std::uint32_t> product_id_;
std::vector<std::uint16_t> region_;
std::vector<std::uint32_t> quantity_;
std::vector<double> price_;
std::vector<std::uint8_t> status_;
std::vector<std::uint64_t> timestamp_;

};
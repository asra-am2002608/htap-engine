#pragma once
#include "order.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class RowStore {
public:
    explicit RowStore(std::size_t reserve_size=0);
    void insert(const Order&order);
    std::optional<Order> lookup(std::uint64_t order_id) const;
    bool update_price(std::uint64_t order_id, double new_price);
    bool update_status(std::uint64_t order_id, std::uint8_t new_status);
    std::size_t size() const;
    const std::vector<Order>& rows() const;
    double sum_price_scalar() const;
    std::size_t count_region_scalar(std::uint16_t region) const;
    std::size_t count_price_greater_than_scalar(double threshold) const;
    const Order& at(std::size_t row_index) const;
    bool update_price_at(std::size_t row_index, double new_price);
    std::size_t estimated_memory_bytes() const;
private:
    std::vector<Order> rows_;
    
};
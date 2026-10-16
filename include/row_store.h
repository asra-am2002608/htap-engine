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

private:
    std::vector<Order> rows_;
    
};
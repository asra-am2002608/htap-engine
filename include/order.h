#pragma once
#include <cstdint>

struct Order{
    std::uint64_t order_id;
    std::uint64_t customer_id;
    std::uint32_t product_id;
    std::uint16_t region;
    std::uint32_t quantity;
    double price;
    std::uint8_t status;
    std::uint64_t timestamp;
};
#pragma once
#include "order.h"
#include <cstdint>
#include <cstddef>
#include <vector>

class DataGenerator {
public:
    explicit DataGenerator(std::uint64_t seed = 42);
    std::vector<Order> generate_orders(std::size_t count);

private:
std::uint64_t seed_;
};
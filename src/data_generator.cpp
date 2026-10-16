#include "data_generator.h"
#include <random>

DataGenerator::DataGenerator(std::uint64_t seed):
    seed_(seed) {}

std::vector<Order> DataGenerator::generate_orders(std::size_t count){
    std::mt19937_64 rng(seed_);
    std::uniform_int_distribution<std::uint64_t> customer_dist(1, 100000);
    std::uniform_int_distribution<std::uint32_t> product_dist(1, 10000);
    std::uniform_int_distribution<std::uint16_t> region_dist(1, 10);
    std::uniform_int_distribution<std::uint32_t> quantity_dist(1, 20);
    std::uniform_real_distribution<double> price_dist(1.0, 1000.0);
    std::uniform_int_distribution<int> status_dist(0, 3);

    std::vector<Order> orders;
    orders.reserve(count);

    for (std::size_t i=0; i<count; ++i){
        Order order{
            static_cast<std::uint64_t> (i+1),
            customer_dist(rng),
            product_dist(rng),
            region_dist(rng),
            quantity_dist(rng),
            price_dist(rng),
            static_cast<std::uint8_t>(status_dist(rng)),
            static_cast<std::uint64_t>(1000000+i)
        };
        orders.push_back(order);
    }
    return orders;
}
#pragma once
#include "order.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class SelectiveColumnStore{
public:
    explicit SelectiveColumnStore(std::size_t reserve_size =0);
    void materialize(const std::vector<Order>& rows);
    std:: size_t size() const;
    double sum_price_scalar() const;

    std:: size_t count_region_scalar(std::uint16_t region) const;
    std:: size_t count_price_greater_than_scalar(double threshold) const;
    std::size_t estimated_memory_bytes() const;

private:
    std::vector<std::uint16_t> region_;
    std::vector<std::uint32_t> quantity_;
    std::vector<double> price_;


};
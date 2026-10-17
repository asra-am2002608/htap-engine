#pragma once
#include "order.h"
#include "column_id.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class SelectiveColumnStore{
public:
    explicit SelectiveColumnStore(std::size_t reserve_size =0);
    void materialize(const std::vector<Order>& rows, const std::vector<ColumnId>& selected_columns);

    std:: size_t size() const;
    bool has_column(ColumnId column) const;
    double sum_price_scalar() const;

    std:: size_t count_region_scalar(std::uint16_t region) const;
    std:: size_t count_price_greater_than_scalar(double threshold) const;
    std::size_t estimated_memory_bytes() const;

    bool update_price_at(std::size_t row_idex, double new_price);
    double price_at(std::size_t row_index) const;

private:
    bool region_materialized_;
    bool quantity_materialized_;
    bool price_materialized_;
    bool status_materialized_;

    std::vector<std::uint16_t> region_;
    std::vector<std::uint32_t> quantity_;
    std::vector<double> price_;
    std::vector<std::uint8_t> status_;


};
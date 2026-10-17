#include "selective_column_store.h"
#include <stdexcept>

SelectiveColumnStore::SelectiveColumnStore(std::size_t reserve_size):
    region_materialized_(false), quantity_materialized_(false), price_materialized_(false), status_materialized_(false){

    if (reserve_size > 0){
        region_.reserve(reserve_size);
        quantity_.reserve(reserve_size);
        price_.reserve(reserve_size);
        status_.reserve(reserve_size);
    }
}

void SelectiveColumnStore::materialize(const std::vector<Order>&rows, const std::vector<ColumnId> & selected_columns){
    region_materialized_=false;
    quantity_materialized_=false;
    price_materialized_=false;
    status_materialized_=false;

    
    
    region_.clear();
    quantity_.clear();
    price_.clear();
    status_.clear();


    for (ColumnId column : selected_columns){
        switch (column){
        case ColumnId::Region:
            region_materialized_=true;
            region_.reserve(rows.size());
            break;
        case ColumnId::Quantity:
            quantity_materialized_=true;
            quantity_.reserve(rows.size());
            break;
        case ColumnId::Price:
            price_materialized_=true;
            price_.reserve(rows.size());
            break;
        case ColumnId::Status:
            status_materialized_=true;
            status_.reserve(rows.size());
            break;
        }

    }

    for (const auto& order : rows){
        if (region_materialized_){
            region_.push_back(order.region);
        }
        if (quantity_materialized_){
            quantity_.push_back(order.quantity);
        }
        if (price_materialized_){
            price_.push_back(order.price);
        }
        if (status_materialized_){
            status_.push_back(order.status);
        }


    }

}

std::size_t SelectiveColumnStore::size() const{
    if(price_materialized_){
        return price_.size();
    }

    if(region_materialized_){
        return region_.size();
    }
    if(quantity_materialized_){
        return quantity_.size();
    }
    if(status_materialized_){
        return status_.size();
    }
    return 0;
}


bool SelectiveColumnStore:: has_column(ColumnId column) const{
    switch (column)
    {
    case ColumnId::Region:
        return region_materialized_;
    case ColumnId::Quantity:
        return quantity_materialized_;
    case ColumnId::Price:
        return price_materialized_;
    case ColumnId::Status:
        return status_materialized_;
    }
        return false;
    
}

double SelectiveColumnStore::sum_price_scalar() const{
    if (!price_materialized_){
        throw std::runtime_error("Price column is not materialized");
    }
    double total = 0.0;
    for (double value:price_){
        total +=value;
    } return total;
}

std::size_t SelectiveColumnStore::count_region_scalar(std::uint16_t region) const{
    if (!region_materialized_){
        throw std::runtime_error("Region column is not materialized");
    }
    std::size_t count = 0;
    for (std::uint16_t value : region_){
        if (value == region){
            ++count;
        }
    } return count;
}

std::size_t SelectiveColumnStore::count_price_greater_than_scalar(double threshold) const{
    if (!price_materialized_){
        throw std::runtime_error("Price column is not materialized");
    }
    std::size_t count = 0;
    for (double value : price_){
        if (value > threshold){
            ++count;
        }
    } return count;
}

std::size_t SelectiveColumnStore::estimated_memory_bytes() const {
    std::size_t total =0;
    if (region_materialized_){
        total+=region_.capacity() *sizeof(std::uint16_t);
    }
    if (quantity_materialized_){
        total+=quantity_.capacity() *sizeof(std::uint32_t);
    }
    if (price_materialized_){
        total+=price_.capacity() *sizeof(double);
    }
    if (status_materialized_){
        total+=status_.capacity() *sizeof(std::uint8_t);
    }
    return total;
}

bool SelectiveColumnStore::update_price_at(std::size_t row_index, double new_price){
    if(!price_materialized_){
        return false;
    }
    if(row_index >= price_.size()){
        return false;
    }
    price_[row_index] =new_price;
    return true;

}


double SelectiveColumnStore::price_at(std::size_t row_index) const{
    if(!price_materialized_){
        throw std::runtime_error("Price column is not materialized");
    }
    if(row_index >=price_.size()){
        throw std::out_of_range("Price row is out of range");

    }
    return price_[row_index];
}
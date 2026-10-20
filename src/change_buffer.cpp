#include "change_buffer.h"
#include "selective_column_store.h"
#include <chrono>

ChangeBuffer::ChangeBuffer(std::size_t batch_size): batch_size_(batch_size){
    changes_.reserve(batch_size);
}

void ChangeBuffer::push(const ChangeRecord& change){
    changes_.push_back(change);
}

bool ChangeBuffer::should_flush() const{
    return changes_.size() >=batch_size_;
}

std::size_t ChangeBuffer::size() const{
    return changes_.size();
}

std::size_t ChangeBuffer::flush(SelectiveColumnStore& column_store, std::vector<double> * freshness_us){
    using Clock = std::chrono::steady_clock;
    std::size_t applied =0;
    for(const auto& change :changes_){
        if(change.column == ColumnId::Price){
            if(column_store.update_price_at(change.row_index, change.numeric_value)){
                ++applied;
                if (freshness_us !=nullptr){
                    const auto visible_time = Clock::now();
                    const double delay = std::chrono::duration<double, std::micro>(visible_time-change.commit_time).count();
                    freshness_us ->push_back(delay);
                }
            }
        }
    }
    changes_.clear();
    return applied;
}
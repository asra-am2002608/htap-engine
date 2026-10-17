#include "change_buffer.h"
#include "selective_column_store.h"

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

std::size_t ChangeBuffer::flush(SelectiveColumnStore& column_store){
    std::size_t applied =0;
    for(const auto& change :changes_){
        if(change.column == ColumnId::Price){
            if(column_store.update_price_at(change.row_index, change.numeric_value)){
                ++applied;
            }
        }
    }
    changes_.clear();
    return applied;
}
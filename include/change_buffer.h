#pragma once
#include "change_record.h"
#include <cstddef>
#include <vector>

class SelectiveColumnStore;

class ChangeBuffer{
public:
    explicit ChangeBuffer(std::size_t batch_size);
    void push(const ChangeRecord& change);
    bool should_flush() const;
    std::size_t size() const;
    std::size_t flush(SelectiveColumnStore& column_store);

private:
    std::size_t batch_size_;
    std::vector<ChangeRecord> changes_;
};
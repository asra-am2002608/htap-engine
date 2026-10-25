#include "workload_runner.h"
#include "metrics.h"
#include "change_buffer.h"
#include "column_id.h"
#include "row_store.h"
#include "selective_column_store.h"
#include <string>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

WorkloadRunner::WorkloadRunner(RowStore& row_store, SelectiveColumnStore& column_store, MetricsCollector& metrics, const std::string& benchmark_mode)
    : row_store_(row_store), column_store_(column_store), metrics_(metrics), benchmark_mode_(benchmark_mode) {}

WorkloadResult WorkloadRunner::run(const WorkloadConfig& config) {
    using Clock = std::chrono::steady_clock;
    WorkloadResult result;

    if (row_store_.size() == 0 ||config.thread_count == 0 ||config.operations_per_thread == 0) {
        return result;
    }

    std::atomic<std::uint64_t> oltp_completed{0};
    std::atomic<std::uint64_t> olap_completed{0};
    std::atomic<std::uint64_t> synchronized_changes{0};

    std::atomic<bool> stop_sync{false};

    std::shared_mutex row_mutex;
    std::shared_mutex column_mutex;

    std::mutex buffer_mutex;

    ChangeBuffer change_buffer(config.sync_batch_size);


    std::thread sync_thread([&]() {
        auto last_flush = Clock::now();

        while (true) {
            const auto now = Clock::now();

            const bool stop_requested =stop_sync.load(std::memory_order_relaxed);
            bool finished = false;

            {
                std::scoped_lock lock(buffer_mutex,column_mutex);

                const bool interval_expired = change_buffer.size() > 0 && (now - last_flush) >= config.sync_interval;

                const bool batch_ready = change_buffer.should_flush();

                const bool final_flush = stop_requested && change_buffer.size() > 0;

                if (batch_ready || interval_expired || final_flush) {
                    std::vector<double>freshness_values;
                    const std::size_t applied = change_buffer.flush(column_store_, &freshness_values);
                    synchronized_changes.fetch_add(applied,std::memory_order_relaxed);
                    for (double freshness : freshness_values){
                        const auto timestamp = Clock::now();
                        const std::uint64_t timestamp_us = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count());
                        metrics_.record(OperationTrace{timestamp_us, 0 , "SYNC", 0.0, 0, freshness});
                    }
                    last_flush = Clock::now();
                }

                if (stop_requested &&change_buffer.size() == 0) {
                    finished = true;
                }
            }

            if (finished) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    const auto start_time =Clock::now();

    std::vector<std::thread> workers;

    workers.reserve(config.thread_count);

    for (std::size_t thread_id = 0; thread_id < config.thread_count;++thread_id) {
        workers.emplace_back([&, thread_id]() {

                std::mt19937_64 rng(42 + thread_id * 1009);
                std::uniform_int_distribution<int>workload_choice(1, 100);
                std::uniform_int_distribution<int>oltp_choice(0, 1);
                std::uniform_int_distribution<std::size_t>row_choice(0,row_store_.size() - 1);

                std::uint64_t local_checksum = 0;

                for (std::size_t operation = 0;operation <config.operations_per_thread;++operation) {
                    const auto operation_start = Clock::now();
                    std::string operation_type;
                    std::size_t trace_row_index =0;
                    const bool is_oltp =workload_choice(rng) <=config.oltp_percent;

                    if (is_oltp) {
                        const std::size_t row_index =row_choice(rng);

                        const bool do_update = oltp_choice(rng) == 1;

                        if (do_update) {
                            operation_type = "OLTP_UPDATE";
                            trace_row_index = row_index;
                            double new_price = 0.0;
                            auto commit_time =Clock::now();

                            {
                                std::unique_lock row_lock(row_mutex);

                                const double old_price =row_store_.at(row_index).price;
                                new_price = old_price + 0.01;
                                row_store_.update_price_at(row_index,new_price);

                                commit_time = Clock::now();

                                if (benchmark_mode_ != "row") {
                                    std::lock_guard buffer_lock(buffer_mutex);
                                    change_buffer.push(ChangeRecord{row_index,ColumnId::Price,new_price,commit_time});
                                }
                            }
                        } else {
                            operation_type = "OLTP_READ";
                            trace_row_index = row_index;
                            std::shared_lock lock(row_mutex);

                            const double observed =row_store_.at(row_index).price;

                            local_checksum += static_cast<std::uint64_t>(observed);
                        }

                        oltp_completed.fetch_add(1,std::memory_order_relaxed);
                    }

                    // --------------------
                    // OLAP
                    // --------------------


                    else {

                        // -----------------------------------------
                        // ROW-ONLY BASELINE
                        // -----------------------------------------
                        if (benchmark_mode_ == "row") {

                            std::shared_lock lock(row_mutex);
                            if (operation % 2 == 0) {
                                operation_type = "OLAP_SUM";
                                const double sum =row_store_.sum_price_scalar();
                                local_checksum += static_cast<std::uint64_t>(sum);
                            }
                            else {
                                operation_type = "OLAP_FILTER";
                                const std::size_t count =row_store_.count_price_greater_than_scalar(500.0);
                                local_checksum += count;
                            }
                        }
                        else if (benchmark_mode_ == "full_scalar"){
                            std::shared_lock lock(column_mutex);
                            if (operation % 2 == 0) {
                                operation_type = "OLAP_SUM";
                                const double sum = column_store_.sum_price_scalar();
                                local_checksum +=static_cast<std::uint64_t>(sum);
                            }
                        
                            else{
                                operation_type = "OLAP_FILTER";
                                const std::size_t count = column_store_.count_price_greater_than_scalar(500);
                                local_checksum +=count;
                            }
                        }
                        // -----------------------------------------
                        // CURRENT SELECTIVE SIMD SYSTEM
                        // -----------------------------------------
                        else {
                            std::shared_lock lock(column_mutex);
                            if (operation % 2 == 0) {
                                operation_type = "OLAP_SUM";
                                const double sum =column_store_.sum_price_simd();
                                local_checksum +=static_cast<std::uint64_t>(sum);
                            }
                            else {
                                operation_type = "OLAP_FILTER";
                                const std::size_t count =column_store_.count_price_greater_than_simd(500.0);
                                local_checksum += count;
                            }
                        }

                        olap_completed.fetch_add(1,std::memory_order_relaxed);
                    }
/*                     else {
                        std::shared_lock lock(column_mutex);

                        if (operation % 2 == 0) {
                            operation_type = "OLAP_SUM";
                            const double sum =column_store_.sum_price_simd();

                            local_checksum += static_cast<std::uint64_t>(sum);

                        } else {
                            operation_type = "OLAP_FILTER";
                            const std::size_t count =column_store_.count_price_greater_than_simd(500.0);
                            local_checksum += count;
                        }

                        olap_completed.fetch_add(1,std::memory_order_relaxed);
                    } */
                    const auto operation_end =Clock::now();
                    const double latency_us =std::chrono::duration<double,std::micro>(operation_end -operation_start).count();
                    const std::uint64_t timestamp_us =static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(operation_end.time_since_epoch()).count());
                    metrics_.record(OperationTrace{timestamp_us,thread_id,operation_type,latency_us,trace_row_index,0.0});

                }
                if (local_checksum == 0) {
                    std::this_thread::yield();
                }
            }
        );
    }


    for (auto& worker : workers) {
        worker.join();
    }

 
    stop_sync.store(true,std::memory_order_relaxed);

    sync_thread.join();

    const auto end_time =Clock::now();

    result.oltp_operations =oltp_completed.load();
    result.olap_operations = olap_completed.load();
    result.synchronized_changes =synchronized_changes.load();
    result.elapsed_seconds =std::chrono::duration<double>(end_time - start_time).count();

    const std::uint64_t total_operations = result.oltp_operations +result.olap_operations;

    if (result.elapsed_seconds > 0.0) {
        result.throughput_ops_per_second =static_cast<double>(total_operations) /result.elapsed_seconds;
        result.oltp_throughput_ops_per_second = static_cast<double>(result.oltp_operations)/result.elapsed_seconds;
        result.olap_throughput_ops_per_second = static_cast<double>(result.olap_operations)/result.elapsed_seconds;
    }

    return result;
}





/*#include "workload_runner.h"
#include "change_buffer.h"
#include "column_id.h"
#include "row_store.h"
#include "selective_column_store.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

WorkloadRunner::WorkloadRunner(RowStore& row_store,SelectiveColumnStore& column_store)
    : row_store_(row_store), column_store_(column_store) {}


WorkloadResult WorkloadRunner::run(const WorkloadConfig& config){
    using Clock = std::chrono::steady_clock;
    WorkloadResult result;

    if (row_store_.size() == 0 || config.thread_count == 0 || config.operations_per_thread == 0) {
        return result;
    }

    std::atomic<std::uint64_t> oltp_completed{0};
    std::atomic<std::uint64_t> olap_completed{0};
    std::atomic<std::uint64_t> synchronized_changes{0};
    std::atomic<bool> stop_sync{false};
    std::shared_mutex row_mutex;
    std::shared_mutex column_mutex;
    std::mutex buffer_mutex;
    ChangeBuffer change_buffer(config.sync_batch_size);

    std::thread sync_thread([&](){
        auto last_flush = Clock::now();
        while (true) {
            const auto now = Clock::now();
            const bool stop_requested = stop_sync.load(std::memory_order_relaxed);
         bool finished = false;
         {
                std::scoped_lock lock(buffer_mutex,column_mutex);

                const bool interval_expired = change_buffer.size() > 0 && (now - last_flush) >= config.sync_interval;
                const bool batch_ready = change_buffer.should_flush();
                const bool final_flush = stop_requested && change_buffer.size() > 0;

                if (batch_ready || interval_expired || final_flush) {
                    const std::size_t applied = change_buffer.flush(column_store_);

                    synchronized_changes.fetch_add(applied, std::memory_order_relaxed);
                    last_flush = Clock::now();
                }
                if (stop_requested && change_buffer.size() == 0) {
                    finished = true;
                }
            }
            if (finished) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });




    const auto start_time =Clock::now();
    std::vector<std::thread> workers;
    workers.reserve(config.thread_count);

    
    for (std::size_t thread_id = 0 ; thread_id < config.thread_count; ++ thread_id ){
        workers.emplace_back([&, thread_id](){
            std::mt19937_64 rng(42 + thread_id * 1009);
            std::uniform_int_distribution<int> oltp_choice(0,1);
            std::uniform_int_distribution<std::size_t>row_choice(0, row_store_.size() -1);
            std::uint64_t local_checksum = 0;
            for(std::size_t operation =0; operation < config.operations_per_thread; ++operation){
                const bool is_oltp = workload_choice(rng) <= config.oltp_percent;
                if(is_oltp){
                    const std::size_t row_index = row_choice(rng);
                    const bool do_update = oltp_choice(rng) ==1;
                    if (do_update){
                        double new_price =0.0;
                        auto commit_time= Clock::now();
                        {
                            std::unique_lock lock(row_mutex);
                            const double old_price = row_store_.at(row_index).price;
                            new_price = old_price + 0.01;
                            row_store_.update_price_at(row_index, new_price);
                            commit_time = Clock::now();
                        }
                        {
                            std::lock_guard lock(buffer_mutex);
                            change_buffer.push(ChangeRecord{row_index, ColumnId::Price, new_price, commit_time});
                        }
                    }
                    else{
                        std::shared_lock lock(row_mutex);
                        const double observed = row_store_.at(row_index).price;
                        local_checksum += static_cast <std::uint64_t>(observed);
                    }
                    oltp_completed.fetch_add(1, std::memory_order_relaxed);
                }
                else{
                    std::shared_lock lock(column_mutex);
                    if(operation % 2 ==0){
                        const double sum = column_store_.sum_price_simd();
                        local_checksum +=static_cast<std::uint64_t>(sum);
                    }
                    else{
                        const std::size_t count = column_store_.count_price_greater_than_simd(500.0);
                        local_checksum +=count;
                    }
                    olap_completed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            if (local_checksum ==0){
                std::this_thread::yield();
            }
        }
    );
    }
    for (auto& worker : workers){
        worker.join();
    }

    stop_sync.store(true, std::memory_order_relaxed);
    sync_thread.join();
    const auto end_time = Clock::now();
    
    result.oltp_operations = oltp_completed.load();
    result.olap_operations = olap_completed.load();
    result.synchronized_changes = synchronized_changes.load();
    result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

    const std::uint64_t total_operarions = result.oltp_operations+ result.olap_operations;
    if(result.elapsed_seconds > 0.0){
        result.throughput_ops_per_second = static_cast<double>(total_operarions) / result.elapsed_seconds;
    }
    return result;
}*/


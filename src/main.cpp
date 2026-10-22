#include <iostream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <fstream>
#include "row_store.h"
#include "data_generator.h"
#include "column_store.h"
#include "selective_column_store.h"
#include "workload_profiler.h"
#include "change_buffer.h"
#include "column_id.h"
#include "workload_runner.h"
#include "metrics.h"

int main(int argc, char* argv[]){
    std::size_t row_count = 1000000;
    std::size_t thread_count = 4;
    std::size_t operations_per_thread = 1000;
    int oltp_percent = 50;
    std::size_t sync_batch_size = 16;
    constexpr std::size_t materialization_threshold = 50;
    /*constexpr std::size_t sync_batch_size=4;*/

    if (argc > 1){
        row_count = static_cast<std::size_t>(std::stoull(argv[1]));
    }
    if (argc > 2){
        thread_count = static_cast<std::size_t>(std::stoull(argv[2]));
    }
    if (argc > 3){
        operations_per_thread = static_cast<std::size_t>(std::stoull(argv[3]));
    }
    if (argc > 4){
        oltp_percent = std::stoi(argv[4]);
    }
    if (argc > 5){
        sync_batch_size = static_cast<std::size_t>(std::stoull(argv[5]));
    }



    DataGenerator generator(42);
    auto orders = generator.generate_orders(row_count);
    RowStore row_store(row_count);
    for(const auto& order: orders){
        row_store.insert(order);
    }

    /*ColumnStore full_column_store(row_count);
    full_column_store.materialize(row_store.rows());*/

    WorkloadProfiler profiler (materialization_threshold);
    for (int i = 0; i < 120; ++i)
    {
        profiler.record_access(ColumnId::Price);
    }
    for (int i = 0; i < 80; ++i)
    {
        profiler.record_access(ColumnId::Region);
    }
        
    for (int i = 0; i < 20; ++i)
    {
        profiler.record_access(ColumnId::Quantity);
    }
    for (int i = 0; i < 5; ++i)
    {
        profiler.record_access(ColumnId::Status);
    }
   
    const auto selected_columns = profiler.selected_columns();


    SelectiveColumnStore column_store(row_count);
    column_store.materialize(row_store.rows(),selected_columns);
    
    
    std::cout << "Rows: "<< row_count<< '\n';

    std::cout << "Price materialized: "<< (column_store.has_column(ColumnId::Price)? "YES": "NO" ) << '\n';
    std::cout << "SIMD available:: " << (SelectiveColumnStore::simd_available() ? "YES" : "NO")  << '\n';

    //std::cout << "Region materialized: " << (column_store.has_column(ColumnId::Region)? "YES": "NO")<< '\n';


    //std::cout << "Rows: " << row_count << '\n';
    //std::cout << "SIMD available:: " << (SelectiveColumnStore::simd_available() ? "YES" : "NO")  << '\n';
    /*constexpr std::size_t target_row = 10;
    const double original_row_price = row_store.at(target_row).price;
    const double original_column_price = selective_column_store.price_at(target_row);*/

    WorkloadConfig config;
    config.thread_count = thread_count;
    config.operations_per_thread = operations_per_thread;
    config.oltp_percent = oltp_percent;
    config.sync_batch_size = sync_batch_size;
    config.sync_interval=std::chrono::milliseconds(5);


    //const double scalar_sum=column_store.sum_price_scalar();
    //const double simd_sum = column_store.sum_price_simd();

    std::cout << "\nMixed Workload Config\n";
    std::cout << "Threads:  " << config.thread_count << '\n';
    std::cout << "Operations per thread:  " << config.operations_per_thread << '\n';
    std::cout << "Target OLTP percentage:  " << config.oltp_percent << "%\n";

    MetricsCollector metrics;
    WorkloadRunner runner(row_store,column_store, metrics);
    const WorkloadResult result = runner.run(config);
    const std::string trace_path = "../results/raw/operations_"+ std::to_string(row_count)+ "_"+
    std::to_string(thread_count) + "_" + std::to_string(oltp_percent) + "_" + std::to_string(sync_batch_size) + ".csv";
    metrics.write_operations_csv(trace_path);

    std::cout << "Raw trace file: " << trace_path << '\n';
    const auto oltp_read_latency = metrics.latency_summary("OLTP_READ");
    const auto oltp_update_latency = metrics.latency_summary("OLTP_UPDATE");
    const auto olap_sum_latency = metrics.latency_summary("OLAP_SUM");
    const auto olap_filter_latency = metrics.latency_summary("OLAP_FILTER");
    


    const std::uint64_t total_operations = result.oltp_operations + result.olap_operations;

    const double actual_oltp_percent= total_operations > 0 ? 100.0 * static_cast<double>(result.oltp_operations) / static_cast<double>(total_operations) : 0.0;
    std::cout << "\nResults\n";
    std::cout << "OLTP operations:  " << result.oltp_operations << '\n';
    std::cout << "OLAP operations:  " << result.olap_operations << '\n';
    std::cout << "Actual OLTP percentage:  " << std::fixed << std::setprecision(2) << actual_oltp_percent << "%\n";
    std::cout << "Synchronized changes:  " << result.synchronized_changes << '\n';
    std::cout << "Elapsed time:  " << result.elapsed_seconds << "s\n";
    std::cout << "Combined throughput:  " << result.throughput_ops_per_second << "ops/s\n";
   
    std::cout << "\nLatency percentile (us)\n";
    std::cout << "OLTP_READ P50/P90/P99: " << oltp_read_latency.p50_us << " / " << oltp_read_latency.p90_us << " / " << oltp_read_latency.p99_us << '\n';
    std::cout << "OLTP_UPDATE P50/P90/P99: " << oltp_update_latency.p50_us << " / " << oltp_update_latency.p90_us << " / " << oltp_update_latency.p99_us << '\n';
    std::cout << "OLAP_SUM P50/P90/P99: " << olap_sum_latency.p50_us << " / " << olap_sum_latency.p90_us << " / " << olap_sum_latency.p99_us << '\n';
    std::cout << "OLAP_FILTER P50/P90/P99: " << olap_filter_latency.p50_us << " / " << olap_filter_latency.p90_us << " / " << olap_filter_latency.p99_us << '\n';

    std::ofstream summary("../results/summary/benchmark_summary.csv", std::ios::app);
    if (!summary) {
        std::cerr<< "Failed to open benchmark summary CSV.\n";
        return 1;
    }
    if (summary.tellp() ==0){
        summary << "rows,"
        << "threads,"
        << "operations_per_thread,"
        << "oltp_percent,"
        << "batch_size,"
        << "oltp_ops,"
        << "olap_ops,"
        << "elapsed_seconds,"
        << "total_throughput,"
        << "oltp_throughput,"
        << "olap_throughput,"
        << "oltp_read_p50,"
        << "oltp_read_p90,"
        << "oltp_read_p99,"
        << "oltp_update_p50,"
        << "oltp_update_p90,"
        << "oltp_update_p99,"
        << "olap_sum_p50,"
        << "olap_sum_p90,"
        << "olap_sum_p99,"
        << "olap_filter_p50,"
        << "olap_filter_p90,"
        << "olap_filter_p99\n";
    }
    summary
    << row_count <<','
    << thread_count << ','
    << operations_per_thread << ','
    << oltp_percent << ','
    << sync_batch_size << ','
    << result.oltp_operations << ','
    << result.olap_operations << ','
    << result.elapsed_seconds << ','
    << result.throughput_ops_per_second << ','
    << result.oltp_throughput_ops_per_second << ','
    << result.olap_throughput_ops_per_second << ','
    << oltp_read_latency.p50_us << ','
    << oltp_read_latency.p90_us << ','
    << oltp_read_latency.p99_us << ','
    << oltp_update_latency.p50_us << ','
    << oltp_update_latency.p90_us << ','
    << oltp_update_latency.p99_us << ','
    << olap_sum_latency.p50_us << ','
    << olap_sum_latency.p90_us << ','
    << olap_sum_latency.p99_us << ','
    << olap_filter_latency.p50_us << ','
    << olap_filter_latency.p90_us << ','
    << olap_filter_latency.p99_us << '\n';

    const double row_sum =row_store.sum_price_scalar();
    const double column_sum =column_store.sum_price_scalar();
    const double differences = std::abs(row_sum- column_sum);
    const double tolerance = std::abs(row_sum) * 1e-10;
    const bool operations_count_correct = total_operations ==config.thread_count * config.operations_per_thread;
    const bool stores_consistent = differences <= tolerance;
    const bool correct = operations_count_correct && stores_consistent;

    std::cout << "\nFinal row/column consistncy:  " << (stores_consistent ? "PASSED" : "FAILED") << '\n';
    std::cout << "\nMixed woarkload correctness:  " << (correct ? "PASSED" : "FAILED") << '\n';
    std::cout << "Raw trace file:" << trace_path << '\n';

    return correct ? 0: 1;
}


   /* const double sum_difference = std::abs(scalar_sum - simd_sum);
    const double sum_tolerance = std::abs(scalar_sum) * 1e-10;
    const bool sum_correct = sum_difference<= sum_tolerance;

    constexpr double threshold = 500.0;
    const std::size_t scalar_count = column_store.count_price_greater_than_scalar(threshold);
    const std::size_t simd_count = column_store.count_price_greater_than_simd(threshold);
    
    std::cout << "\nCount Price (price > 500)\n";
    std::cout << "Scalar " << scalar_count<< '\n';
    std::cout << "SIMD " << simd_count << '\n';
    const bool count_correct = scalar_count ==simd_count;

    const bool correct = sum_correct && count_correct; 
    
    std::cout <<"\nSIMD correctness: " << (correct ? "Passed " : "Failed ") << '\n';
    return correct ? 0:1;
}*/


/*
    std::cout << "Before update \n";
    std::cout << "Row-store price:  " << original_row_price << '\n';
    std::cout << "Column-store price:  " << original_column_price << '\n';

    const double new_price = original_row_price+100.0;
    const auto commit_time = std::chrono::steady_clock::now();
    const std::uint64_t order_id = row_store.at(target_row).order_id;

    row_store.update_price(order_id, new_price);

    change_buffer.push(ChangeRecord{
        target_row, ColumnId::Price, new_price, commit_time

    });


    const  double stale_column_price = selective_column_store.price_at(target_row);
    std::cout << "\nAfter OLTP update, before flush \n";
    std::cout << "Row-store price:  " << row_store.at(target_row).price << '\n';
    std::cout << "Column-store price:  " << stale_column_price << '\n';

    if (change_buffer.should_flush()){
        change_buffer.flush(selective_column_store);
    }

    std::cout << "\n Buffer size after 1 update:  " << change_buffer.size() << '\n';


    const std::size_t applied = change_buffer.flush(selective_column_store);
    const auto visible_time = std::chrono::steady_clock::now();
    const auto freshness_us = std::chrono::duration_cast<std::chrono::microseconds>(
        visible_time - commit_time).count();
    const double syncronized_price = selective_column_store.price_at(target_row);

    std::cout << "\nAfteer Syncronization\n";
    std::cout << "Row-store price:  " << row_store.at(target_row).price << '\n';
    std::cout << "Column-store price:  " << syncronized_price << '\n';
    std::cout << "Changes applied: " << applied << '\n';
    std::cout << "freshness delay: " << freshness_us << "us\n";

    const bool stale_before_flush = stale_column_price != new_price;
    const bool synchronized_after_flush = syncronized_price == new_price;
    const bool correct = stale_before_flush && synchronized_after_flush &&applied ==1;
    std::cout <<"\nSynchronization correctness: " << (correct ? "Passed " : "Failed ") << '\n';

    return correct ? 0:1;


    std::cout << "Workload access counts \n";
    std::cout << "Price: " << profiler.access_count(ColumnId::Price) << '\n';
    std::cout << "Region: " << profiler.access_count(ColumnId::Region) << '\n';
    std::cout << "Quantity: " << profiler.access_count(ColumnId::Quantity) << '\n';
    std::cout << "Status: " << profiler.access_count(ColumnId::Status) << '\n';

    std::cout << "\nMaterialaztion threshold: \n" << materialization_threshold  << '\n';
    std::cout << "\nSelected Columns\n";

    std::cout << "Price: " << (selective_column_store.has_column(ColumnId::Price) ? "YES" : "NO") << '\n';
    std::cout << "Region: " << (selective_column_store.has_column(ColumnId::Region) ? "YES" : "NO") << '\n';
    std::cout << "Quantity: " << (selective_column_store.has_column(ColumnId::Quantity) ? "YES" : "NO") << '\n';
    std::cout << "Status: " << (selective_column_store.has_column(ColumnId::Status) ? "YES" : "NO") << '\n';

    bool correct = true;
    if (!selective_column_store.has_column(ColumnId::Price)){
        correct = false;
    }
    if (!selective_column_store.has_column(ColumnId::Region)){
        correct = false;
    }
    if (selective_column_store.has_column(ColumnId::Quantity)){
        correct = false;
    }
    if (selective_column_store.has_column(ColumnId::Status)){
        correct = false;
    }


    const double row_sum = row_store.sum_price_scalar();
    const double selective_sum = selective_column_store.sum_price_scalar();
    if (std::abs(row_sum - selective_sum) > 0.0001){
        correct = false;
    }

    const std::size_t row_region_count = row_store.count_region_scalar(5);
    const std::size_t selective_region_count = selective_column_store.count_region_scalar(5);
    if (row_region_count!=selective_region_count){
        correct = false;
    }
    const std::size_t full_bytes = full_column_store.estimated_memory_bytes();
    const std::size_t selective_bytes = selective_column_store.estimated_memory_bytes();

    const double full_mb =static_cast<double>(full_bytes) / (1024.0 * 1024.0);
    const double selective_mb =static_cast<double>(selective_bytes) / (1024.0 * 1024.0);
    const double reduction = 100.0 * (1.0 -static_cast<double>(selective_bytes) / static_cast<double>(full_bytes));

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nColumn-store memory estimate\n";

    std::cout <<"Full column store : " << full_mb << "MB\n";
    std::cout <<"Workload-aware store : " << selective_mb << "MB\n";
    std::cout <<"Estimated reduction : " << reduction << "%\n";

    std::cout << "\nCorrectness chaeck: \n" << (correct ? "Passed" : "Failed")  << '\n';

    return correct ? 0:1;

}*/
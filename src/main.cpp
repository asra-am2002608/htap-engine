#include <iostream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
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
    std::size_t run_id=1;
    std::string benchmark_mode = "selective_simd";
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
    if (argc >6){
        run_id = static_cast<std::size_t>(std::stoull(argv[6]));
    }
    if (argc >7){
        benchmark_mode = argv[7];
    }

    if (benchmark_mode != "row" && benchmark_mode != "full_scalar" && benchmark_mode != "full_simd" && benchmark_mode != "selective_simd"){
        std::cerr << "Invalid benchmark mode: " << benchmark_mode << '\n';
        std::cerr << "Valid modes: " << "row, " << "full_scalar, " << "full_simd, " << "selective_simd\n";
        return 1;
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
   

    std::vector<ColumnId> selected_columns;
    if(benchmark_mode == "row"){
        selected_columns.clear();
    }
    else if( benchmark_mode =="full_scalar" || benchmark_mode == "full_simd"){
        selected_columns = { ColumnId::Region, ColumnId::Quantity, ColumnId::Price, ColumnId::Status};
    }
    else {
        selected_columns = profiler.selected_columns();
    }
    //const auto selected_columns = profiler.selected_columns();


    SelectiveColumnStore column_store(row_count);
    column_store.materialize(row_store.rows(),selected_columns);
    
    const std::size_t row_store_bytes = row_store.estimated_memory_bytes();
    const std::size_t column_store_bytes = benchmark_mode == "row" ? 0 : column_store.estimated_memory_bytes();
    const std::size_t total_estimated_bytes = row_store_bytes + column_store_bytes;
    const double row_store_mb = static_cast<double>(row_store_bytes) / (1024.0 * 1024.0);
    const double column_store_mb = static_cast<double>(column_store_bytes) / (1024.0 * 1024.0);
    const double total_estimated_mb = static_cast<double>(total_estimated_bytes) / (1024.0 * 1024.0);


    const std::size_t full_replica_bytes = row_count * (sizeof(std::uint16_t)+ sizeof(std::uint32_t)+ sizeof(double) + sizeof(std::uint8_t));
    double replica_reduction_percent = 0.0;
    if (benchmark_mode == "selective_simd" && full_replica_bytes >0){
        replica_reduction_percent = 100.0 * ( 1.0 - static_cast<double>(column_store_bytes)/static_cast<double>(full_replica_bytes));
    
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nMemory estimate\n";
    std::cout << "Row store: " << row_store_mb << "MB\n";
    std::cout << "Analytical replica:" << column_store_mb << "MB\n";
    std::cout << "Total estimate storage: " << total_estimated_mb << "MB\n";

    if (benchmark_mode == "selective_simd"){
        std::cout << "Analytical replica reduction vs full:" << replica_reduction_percent << "%\n";
    }

    std::cout << "Rows: "<< row_count<< '\n';
    std::cout << "Price materialized: "<< (column_store.has_column(ColumnId::Price)? "YES": "NO" ) << '\n';
    std::cout << "SIMD available:: " << (SelectiveColumnStore::simd_available() ? "YES" : "NO")  << '\n';
    std::cout << "Quantity materialized: "<< (column_store.has_column(ColumnId::Quantity)? "YES": "NO" ) << '\n';
    std::cout << "Status materialized: "<< (column_store.has_column(ColumnId::Status)? "YES": "NO" ) << '\n';



    //std::cout << "Region materialized: " << (column_store.has_column(ColumnId::Region)? "YES": "NO")<< '\n';


    
    
    std::cout << "Rows: " << row_count << '\n';
    std::cout << "Run: " << run_id << '\n';
    std::cout << "benchmark mode: " << benchmark_mode << '\n';
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
    WorkloadRunner runner(row_store,column_store, metrics, benchmark_mode);
    const WorkloadResult result = runner.run(config);
    const std::string trace_path = "../results/raw/operations_"+ benchmark_mode+ "_" +std::to_string(row_count)+ "_"+
    std::to_string(thread_count) + "_" + std::to_string(oltp_percent) + "_" + std::to_string(sync_batch_size) + "_run" + std::to_string(run_id)+ ".csv";
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
        summary << "mode,"
        << "run,"
        << "rows,"
        << "thread,"
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
        << "olap_filter_p99,"
        << "row_store_bytes,"
        << "column_store_bytes,"
        << "total_estimated_bytes,"
        << "replica_reduction_percent"
        << '\n';
    }
    summary
    << benchmark_mode << ','
    << run_id << ','
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
    << olap_filter_latency.p99_us << ','
    << row_store_bytes << ','
    << column_store_bytes << ','
    << total_estimated_bytes << ','
    << replica_reduction_percent << '\n';

    //const double row_sum =row_store.sum_price_scalar();
    //const double column_sum =column_store.sum_price_scalar();
    //const double differences = std::abs(row_sum- column_sum);
    //const double tolerance = std::abs(row_sum) * 1e-10;
    
    
    const bool operations_count_correct = total_operations == config.thread_count * config.operations_per_thread;
    bool stores_consistent = true;

    if (benchmark_mode == "row") {
        std::cout<< "\nRow-only execution check: "<< (operations_count_correct ? "PASSED" : "FAILED")<< '\n';
    }
    else {
        const double row_sum =row_store.sum_price_scalar();
        const double column_sum =column_store.sum_price_scalar();
        const double difference =std::abs(row_sum - column_sum);
        const double tolerance =std::abs(row_sum) * 1e-10;
        stores_consistent =difference <= tolerance;
        std::cout << "\nFinal row/column consistency: " << (stores_consistent ? "PASSED" : "FAILED") << '\n';
    }

    const bool correct = operations_count_correct && stores_consistent;

    std::cout << "\nMixed workload correctness: " << (correct ? "PASSED" : "FAILED") << '\n';
    std::cout << "Raw trace file: "<< trace_path<< '\n';
    return correct ? 0 : 1;
    

}


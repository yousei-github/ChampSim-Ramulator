#include "simulator_statistics.h"

#if (USER_CODES == ENABLE)

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>
#endif /* USE_VCPKG */

#include "ChampSim/champsim_constants.h"

// Functions private to a Compilation Unit (TU - Translation Unit): using anonymous namespaces or the static keyword
namespace
{

/**
 * Linux file systems cap a single path component at NAME_MAX (typically 255).
 * If the basename of `path` would exceed that, replace its middle with a
 * deterministic hash so the resulting name is short, unique per original
 * input, and keeps the original extension.
*/
std::string shorten_path_if_needed(const std::string& path)
{
    const size_t slash     = path.find_last_of('/');
    const std::string dir  = (slash == std::string::npos) ? "" : path.substr(0, slash + 1); // Directory
    const std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);  // Base name

    if (base.size() <= NAME_MAX)
        return path;

    const size_t dot       = base.find_last_of('.');
    const std::string ext  = (dot == std::string::npos) ? "" : base.substr(dot);      // Extension
    const std::string stem = (dot == std::string::npos) ? base : base.substr(0, dot); // Stem

    char hash_suffix[18]; // "_" + 16 hex + NUL
    std::snprintf(hash_suffix, sizeof(hash_suffix), "_%016zx", std::hash<std::string> {}(base));

    const size_t budget              = NAME_MAX - ext.size() - (sizeof(hash_suffix) - 1);
    const std::string shortened_stem = stem.substr(0, std::min(stem.size(), budget));

    return dir + shortened_stem + hash_suffix + ext;
}

/**
 * Tabulate the effective configuration of every cache the caller collected. The geometry is derived
 * by champsim::cache_builder from the macros in ChampSim/champsim_constants.h, so this table is the
 * only place a build reveals what it actually simulates.
 */
std::string format_cache_configuration(const std::vector<CACHE_CONFIGURATION>& cache_configuration)
{
    std::ostringstream stream;

    stream << "Cache configuration:\n"
           << std::setw(12) << "NAME" << std::setw(8) << "SETS" << std::setw(6) << "WAYS" << std::setw(7) << "MSHRS" << std::setw(6) << "PQ"
           << std::setw(9) << "HIT_LAT" << std::setw(10) << "FILL_LAT" << std::setw(9) << "MAX_TAG" << std::setw(10) << "MAX_FILL" << '\n';

    for (const CACHE_CONFIGURATION& cache : cache_configuration)
    {
        stream << std::setw(12) << cache.name << std::setw(8) << cache.sets << std::setw(6) << cache.ways << std::setw(7) << cache.mshrs
               << std::setw(6) << cache.pq_size << std::setw(9) << cache.hit_latency_in_cycles << std::setw(10) << cache.fill_latency_in_cycles
               << std::setw(9) << cache.max_tag_check << std::setw(10) << cache.max_fill << '\n';
    }

    return stream.str();
}
} // namespace

MEMORY_TRACE output_memorytrace("memory trace", ".trace");
SIMULATOR_STATISTICS output_statistics("ChampSim statistics", ".statistics");

DATA_OUTPUT::DATA_OUTPUT(std::string v1, std::string v2)
: data_name(v1), file_extension(v2)
{
}

DATA_OUTPUT::DATA_OUTPUT(std::string v1, std::string v2, const char* string)
: data_name(v1), file_extension(v2)
{
    output_file_initialization(string);
}

DATA_OUTPUT::DATA_OUTPUT(std::string v1, std::string v2, char** string_array, uint32_t number)
: data_name(v1), file_extension(v2)
{
    output_file_initialization(string_array, number);
}

DATA_OUTPUT::~DATA_OUTPUT()
{
    close_file(file_handler);

    if (file_name) // Check the validity of this file name string
    {
        printf("Output %s into %s\n", data_name.c_str(), file_name);
        free(file_name);
    }
}

void DATA_OUTPUT::output_file_initialization(const char* string)
{
    close_file(file_handler); // In case of repeated file open

    const std::string original_name = string;
    const std::string actual_name   = shorten_path_if_needed(original_name);

    if (actual_name != original_name)
    {
        std::cerr << __func__ << ": output filename '" << original_name << "' (" << original_name.size() << " bytes) exceeds NAME_MAX; shortened to '" << actual_name << "'." << std::endl;
    }

    file_handler = fopen(actual_name.c_str(), "w");
    if (file_handler == nullptr)
    {
        std::cerr << __func__ << ": File Open Error opening '" << actual_name << "': " << strerror(errno) << std::endl;
        std::abort();
    }

    file_name = (char*) malloc(actual_name.size() + 1);

    if (file_name == nullptr)
    {
        std::cerr << __func__ << ": Memory Allocation Error." << std::endl;
        std::abort();
    }

    strcpy(file_name, actual_name.c_str());
}

void DATA_OUTPUT::output_file_initialization(char** string_array, uint32_t number)
{
    close_file(file_handler); // In case of repeated file open

    std::string benchmark_names;
    for (uint32_t i = 0; i < number; i++)
    {
        const std::string path     = string_array[i];
        const size_t slash         = path.find_last_of('/');
        const std::string basename = (slash == std::string::npos) ? path : path.substr(slash + 1);

        if (basename.empty())
        {
            std::cerr << __func__ << ": Empty basename for input '" << path << "'." << std::endl;
            std::abort();
        }

        benchmark_names += basename + "_";
    }
    benchmark_names.erase(benchmark_names.size() - 1); // Delete the last "_"

    // Append file_extension to benchmark_names.
    benchmark_names += file_extension.c_str();

    const std::string actual_name = shorten_path_if_needed(benchmark_names);

    if (actual_name != benchmark_names)
    {
        std::cerr << __func__ << ": output filename '" << benchmark_names << "' (" << benchmark_names.size() << " bytes) exceeds NAME_MAX; shortened to '" << actual_name << "'." << std::endl;
    }

    file_handler = fopen(actual_name.c_str(), "w");
    if (file_handler == nullptr)
    {
        std::cerr << __func__ << ": File Open Error opening '" << actual_name << "': " << strerror(errno) << std::endl;
        std::abort();
    }

    file_name = (char*) malloc(actual_name.size() + 1);

    if (file_name == nullptr)
    {
        std::cerr << __func__ << ": Memory Allocation Error." << std::endl;
        std::abort();
    }

    strcpy(file_name, actual_name.c_str());
}

void DATA_OUTPUT::close_file(FILE* file_handler)
{
    if (file_handler) // Check the validity of this file handler
    {
        fclose(file_handler);
    }
}

MEMORY_TRACE::MEMORY_TRACE(std::string v1, std::string v2)
: DATA_OUTPUT(v1, v2)
{
}

MEMORY_TRACE::MEMORY_TRACE(std::string v1, std::string v2, char** string_array, uint32_t number)
: DATA_OUTPUT(v1, v2, string_array, number)
{
}

void MEMORY_TRACE::output_memory_trace_hexadecimal(uint64_t address, char type)
{
    assert(file_handler);
    fprintf(file_handler, "0x%lx %c\n", address, type);
}

SIMULATOR_STATISTICS::SIMULATOR_STATISTICS(std::string v1, std::string v2)
: DATA_OUTPUT(v1, v2)
{
    statistics_initialization();
}

SIMULATOR_STATISTICS::SIMULATOR_STATISTICS(std::string v1, std::string v2, char** string_array, uint32_t number)
: DATA_OUTPUT(v1, v2, string_array, number)
{
    statistics_initialization();
}

SIMULATOR_STATISTICS::~SIMULATOR_STATISTICS()
{
    if (file_handler) // Check the validity of this file handler
    {
        fprintf(file_handler, "\n\nInformation about virtual memory\n\n");
        for (uint64_t i = 0; i < valid_pte_count.size(); i++)
        {
            fprintf(file_handler, "Level: %ld, valid_pte_count: %ld.\n", i, valid_pte_count[i]);
        }
        fprintf(file_handler, "virtual_page_count: %ld, main memory footprint: %f MB.\n", virtual_page_count, virtual_page_count * 4.0 / KiB);

        uint64_t total_access_request_in_memory = read_request_in_memory + read_request_in_memory2 + write_request_in_memory + write_request_in_memory2;
        if (total_access_request_in_memory == 0)
        {
            total_access_request_in_memory = 1; /** @todo Output "-" when total_access_request_in_memory is 0 */
        }

        fprintf(file_handler, "\n\nInformation about memory controller (For Ramulator)\n\n");
        fprintf(file_handler, "read_request_in_memory: %ld, read_request_in_memory2: %ld.\n", read_request_in_memory, read_request_in_memory2);
        fprintf(file_handler, "write_request_in_memory: %ld, write_request_in_memory2: %ld.\n", write_request_in_memory, write_request_in_memory2);
        fprintf(file_handler, "hit rate: %f.\n", (read_request_in_memory + write_request_in_memory) / float(total_access_request_in_memory));
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
        fprintf(file_handler, "load_request_in_memory: %ld, load_request_in_memory2: %ld.\n", load_request_in_memory, load_request_in_memory2);
        fprintf(file_handler, "store_request_in_memory: %ld, store_request_in_memory2: %ld.\n", store_request_in_memory, store_request_in_memory2);
#endif /* TRACKING_LOAD_STORE_STATISTICS */

        fprintf(file_handler, "swapping_count: %ld, swapping_traffic_in_bytes: %ld.\n", swapping_count, swapping_traffic_in_bytes);

        fprintf(file_handler, "remapping_request_queue_congestion: %ld.\n", remapping_request_queue_congestion);

#if (IDEAL_VARIABLE_GRANULARITY == ENABLE)
        fprintf(file_handler, "no_free_space_for_migration: %ld (%f).\n", no_free_space_for_migration, no_free_space_for_migration / float(total_access_request_in_memory));
        fprintf(file_handler, "no_invalid_group_for_migration: %ld (%f).\n", no_invalid_group_for_migration, no_invalid_group_for_migration / float(total_access_request_in_memory));
        fprintf(file_handler, "unexpandable_since_start_address: %ld (%f).\n", unexpandable_since_start_address, unexpandable_since_start_address / float(total_access_request_in_memory));
        fprintf(file_handler, "unexpandable_since_no_invalid_group: %ld (%f).\n", unexpandable_since_no_invalid_group, unexpandable_since_no_invalid_group / float(total_access_request_in_memory));
        fprintf(file_handler, "data_eviction_success: %ld (%f).\n", data_eviction_success, data_eviction_success / float(total_access_request_in_memory));
        fprintf(file_handler, "data_eviction_failure: %ld (%f).\n", data_eviction_failure, data_eviction_failure / float(total_access_request_in_memory));
        fprintf(file_handler, "uncertain_counter: %ld (%f).\n", uncertain_counter, uncertain_counter / float(total_access_request_in_memory));
#endif /* IDEAL_VARIABLE_GRANULARITY */
    }
}

void SIMULATOR_STATISTICS::print_simulation_start(long long warmup_instructions, long long simulation_instructions, std::size_t cpu_number,
    const std::vector<CACHE_CONFIGURATION>& cache_configuration)
{
    std::ostringstream stream;

    stream << "\n*** ChampSim Multicore Out-of-Order Simulator ***\n"
           << "Warmup Instructions: " << warmup_instructions << '\n'
           << "Simulation Instructions: " << simulation_instructions << '\n'
           << "Number of CPUs: " << cpu_number << '\n'
           << "Page size: " << PAGE_SIZE << "\n\n"
           << format_cache_configuration(cache_configuration) << '\n';

    const std::string text = stream.str();

#if (USE_VCPKG == ENABLE)
    fmt::print("{}", text);
#endif /* USE_VCPKG */

    // file_handler stays nullptr unless output_file_initialization() opened the statistics file,
    // which only happens when PRINT_STATISTICS_INTO_FILE is enabled.
    if (file_handler != nullptr)
    {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fputs(text.c_str(), file_handler);
#endif /* PRINT_STATISTICS_INTO_FILE */
    }
}

void SIMULATOR_STATISTICS::statistics_initialization()
{
    /* Initialize variabes */
    for (uint64_t i = 0; i < valid_pte_count.size(); i++)
    {
        valid_pte_count[i] = 0;
    }
    virtual_page_count = 0;

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    load_request_in_memory   = 0;
    load_request_in_memory2  = 0;
    store_request_in_memory  = 0;
    store_request_in_memory2 = 0;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    read_request_in_memory             = 0;
    read_request_in_memory2            = 0;
    write_request_in_memory            = 0;
    write_request_in_memory2           = 0;

    swapping_count                     = 0;
    swapping_traffic_in_bytes          = 0;

    remapping_request_queue_congestion = 0;

#if (IDEAL_VARIABLE_GRANULARITY == ENABLE)
    no_free_space_for_migration         = 0;
    no_invalid_group_for_migration      = 0;
    unexpandable_since_start_address    = 0;
    unexpandable_since_no_invalid_group = 0;
    data_eviction_success = data_eviction_failure = 0;
    uncertain_counter                             = 0;
#endif /* IDEAL_VARIABLE_GRANULARITY */
}

#endif /* USER_CODES */

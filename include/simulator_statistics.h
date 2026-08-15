#ifndef SIMULATOR_STATISTICS_H
#define SIMULATOR_STATISTICS_H

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

/* Header */

#include <stdint.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "ChampSim/champsim_constants.h" // For PAGE_TABLE_LEVELS

/* Macro */

/**
 * Print a printf-style line into the simulator statistics file.
 * It expands to nothing when PRINT_STATISTICS_INTO_FILE is disabled, so call sites need no guard.
 *
 * The null check matters for binaries that never call output_file_initialization(), such as the unit
 * tests: there the handler legitimately stays null, and std::fprintf(nullptr, ...) is undefined behaviour.
 * The simulator opens the file before any call site can run, so this costs it one predictable branch.
 */
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
#define PRINTF_STATISTICS_FILE(...)                                                                    \
    ((output_statistics.file_handler != nullptr) ? (void) std::fprintf(output_statistics.file_handler, \
                                                       __VA_ARGS__)                                    \
                                                 : (void) 0)
#else
#define PRINTF_STATISTICS_FILE(...) ((void) 0)
#endif /* PRINT_STATISTICS_INTO_FILE */

/* Type */

/**
 * One row of the startup cache-configuration table. It deliberately holds only primitives, so this
 * header stays free of ChampSim types even though nearly every translation unit includes it.
 * source/main.cc fills the rows from champsim::environment.
 */
struct CACHE_CONFIGURATION
{
    std::string name;
    uint32_t sets;
    uint32_t ways;
    uint32_t mshrs;
    std::size_t pq_size;
    long long hit_latency_in_cycles;
    long long fill_latency_in_cycles;
    long long max_tag_check;
    long long max_fill;
};

/* Prototype */

/**
 * The primitive class for data output.
 */
class DATA_OUTPUT
{
public:
    const std::string data_name;      // The name of data to output
    const std::string file_extension; // The name of file extension
    FILE* file_handler = nullptr;
    char* file_name    = nullptr;

    DATA_OUTPUT(std::string v1, std::string v2);
    DATA_OUTPUT(std::string v1, std::string v2, const char* string);
    DATA_OUTPUT(std::string v1, std::string v2, char** string_array, uint32_t number);
    ~DATA_OUTPUT();

    /**
     * Initialize the output file's name based on the name of input @p string.
     *
     * @param[in] string The name string.
     */
    void output_file_initialization(const char* string);

    /**
     * Initialize the output file's name based on the name of input @p string_array[number].
     * It extracts the last name of each string from @p string_array using the delimiter "/" and concatenates them to form a single string as the result.
     *
     * @param[in] string_array The string array containing a string.
     * @param[in] number The number of strings in the string_array.
     */
    void output_file_initialization(char** string_array, uint32_t number);

private:
    void close_file(FILE* file_handler);
};

// Memory trace output class
class MEMORY_TRACE : public DATA_OUTPUT
{
public:
    MEMORY_TRACE(std::string v1, std::string v2);
    MEMORY_TRACE(std::string v1, std::string v2, char** string_array, uint32_t number);

    void output_memory_trace_hexadecimal(uint64_t address, char type);
};

// Simulator statistics output class
class SIMULATOR_STATISTICS : public DATA_OUTPUT
{
public:
#define PAGE_TABLE_LEVEL_NUMBER (PAGE_TABLE_LEVELS)

    std::array<uint64_t, PAGE_TABLE_LEVEL_NUMBER> valid_pte_count = {0};
    uint64_t virtual_page_count;

    uint64_t read_request_in_memory, read_request_in_memory2;
    uint64_t write_request_in_memory, write_request_in_memory2;

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    uint64_t load_request_in_memory, load_request_in_memory2;
    uint64_t store_request_in_memory, store_request_in_memory2;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    uint64_t swapping_count;
    uint64_t swapping_traffic_in_bytes;

    uint64_t remapping_request_queue_congestion;

#if (IDEAL_VARIABLE_GRANULARITY == ENABLE)
    uint64_t no_free_space_for_migration;
    uint64_t no_invalid_group_for_migration;
    uint64_t unexpandable_since_start_address;
    uint64_t unexpandable_since_no_invalid_group;
    uint64_t data_eviction_success, data_eviction_failure;
    uint64_t uncertain_counter;
#endif /* IDEAL_VARIABLE_GRANULARITY */

    SIMULATOR_STATISTICS(std::string v1, std::string v2);
    SIMULATOR_STATISTICS(std::string v1, std::string v2, char** string_array, uint32_t number);
    ~SIMULATOR_STATISTICS();

    /**
     * Print the run start message and the effective cache configuration to stdout, and to the
     * statistics file as well when one is open (see output_file_initialization).
     *
     * @param[in] warmup_instructions The number of instructions used to warm up the simulation.
     * @param[in] simulation_instructions The number of instructions to simulate.
     * @param[in] cpu_number The number of simulated CPUs.
     * @param[in] cache_configuration One row per cache, in the order they should be tabulated.
     */
    void print_simulation_start(long long warmup_instructions, long long simulation_instructions, std::size_t cpu_number,
        const std::vector<CACHE_CONFIGURATION>& cache_configuration);

private:
    void statistics_initialization();
};

/* Variable */

extern MEMORY_TRACE output_memorytrace;
extern SIMULATOR_STATISTICS output_statistics;

/* Function */

#endif /* USER_CODES */

#endif

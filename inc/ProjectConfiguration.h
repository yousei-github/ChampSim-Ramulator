#ifndef __PROJECTCONFIGURATION_H
#define __PROJECTCONFIGURATION_H

#define ENABLE  (1)
#define DISABLE (0)

#define DEBUG_PRINTF (DISABLE)
#define USER_CODES   (ENABLE)

/* Functionality options */
#if (USER_CODES == ENABLE)
// Main functionalities selection
#define USE_OPENMP                                 (ENABLE) // whether use OpenMP to speedup the simulation
#define RAMULATOR                                  (ENABLE) // whether use ramulator, assuming ramulator uses addresses at byte granularity and returns data at cache line granularity.
#define MEMORY_USE_HYBRID                          (ENABLE) // whether use hybrid memory system instead of single memory systems
#define PRINT_STATISTICS_INTO_FILE                 (ENABLE) // whether print simulation statistics into files
#define PRINT_MEMORY_TRACE                         (ENABLE) // whether print memory trace into files
#define MEMORY_USE_SWAPPING_UNIT                   (ENABLE) // whether memory controller uses swapping unit to swap data (data swapping overhead is considered)
#define MEMORY_USE_OS_TRANSPARENT_MANAGEMENT       (ENABLE) // whether memory controller uses OS-transparent management designs to simulate the memory system instead of static (no-migration) methods
#define CPU_USE_MULTIPLE_CORES                     (ENABLE) // whether CPU uses multiple cores to run simulation (go to ./inc/ChampSim/champsim_constants.h to check related parameters)

// Configuration for hybrid memory systems
#if (MEMORY_USE_HYBRID == ENABLE)
#define NUMBER_OF_MEMORIES   (2u)    // we use two memories for hybrid memory system.
#define MEMORY_NUMBER_ONE    (0u)
#define MEMORY_NUMBER_TWO    (1u)
#define ADD_HBM_128MB        (ENABLE)
#else
#define NUMBER_OF_MEMORIES   (1u)
#endif  // MEMORY_USE_HYBRID

// Configuration for swapping unit in the memory controller
#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
#define SWAPPING_BUFFER_ENTRY_NUMBER    (64)
#define SWAPPING_SEGMENT_ONE            (0)
#define SWAPPING_SEGMENT_TWO            (1)
#define SWAPPING_SEGMENT_NUMBER         (2)
#define TEST_SWAPPING_UNIT              (DISABLE)
#endif  // MEMORY_USE_SWAPPING_UNIT

/* Research proposal selection */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#define IDEAL_LINE_LOCATION_TABLE             (ENABLE)
#define COLOCATED_LINE_LOCATION_TABLE         (DISABLE)
#define IDEAL_VARIABLE_GRANULARITY            (DISABLE)
#define IDEAL_SINGLE_MEMPOD                   (DISABLE)

#define TRACKING_LOAD_STORE_STATISTICS        (ENABLE)

#if (IDEAL_LINE_LOCATION_TABLE == DISABLE) && (COLOCATED_LINE_LOCATION_TABLE == DISABLE) && (IDEAL_VARIABLE_GRANULARITY == DISABLE) && (IDEAL_SINGLE_MEMPOD == DISABLE)
#define NO_METHOD_FOR_RUN_HYBRID_MEMORY       (ENABLE)
#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_VARIABLE_GRANULARITY, IDEAL_SINGLE_MEMPOD

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)  // Note: it might be better become part of configurations of TRACKING_LOAD_STORE_STATISTICS like IDEAL_SINGLE_MEMPOD in line 71
/* Option for research */
#define TRACKING_LOAD_ONLY                    (DISABLE)
#define TRACKING_READ_ONLY                    (DISABLE)
#endif // TRACKING_LOAD_STORE_STATISTICS

// Configuration for each research proposal
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
#define HOTNESS_THRESHOLD                     (1u)
#define BITS_MANIPULATION                     (DISABLE)
#elif (IDEAL_VARIABLE_GRANULARITY == ENABLE)
#define HOTNESS_THRESHOLD                     (1u)      // default: 1/4
#define DATA_EVICTION                         (ENABLE)
#define FLEXIBLE_DATA_PLACEMENT               (ENABLE)
#define STATISTICS_INFORMATION                (ENABLE)
#define FLEXIBLE_GRANULARITY                  (ENABLE)
#define IMMEDIATE_EVICTION                    (DISABLE)
#define COLD_DATA_DETECTION_IN_GROUP          (DISABLE)
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
#define PRINT_SWAPS_PER_EPOCH_MEMPOD          (DISABLE)
#else
#define HOTNESS_THRESHOLD                     (1u)
#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_VARIABLE_GRANULARITY, IDEAL_SINGLE_MEMPOD

// Check
#if (NO_METHOD_FOR_RUN_HYBRID_MEMORY == ENABLE)
#error OS-transparent management designs need to be enabled.
#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE

#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (PRINT_MEMORY_TRACE == ENABLE)
#define CONTINUOUS_ADDRESS                    (ENABLE)
#endif  // PRINT_MEMORY_TRACE

// Data block management granularity
#define DATA_GRANULARITY_64B                (64u)
#define DATA_GRANULARITY_128B               (128u)
#define DATA_GRANULARITY_256B               (256u)
#define DATA_GRANULARITY_512B               (512u)
#define DATA_GRANULARITY_1024B              (1024u)
#define DATA_GRANULARITY_2048B              (2048u)
#define DATA_GRANULARITY_4096B              (4096u)

#if (IDEAL_SINGLE_MEMPOD == DISABLE)
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
#define DATA_MANAGEMENT_GRANULARITY         (DATA_GRANULARITY_64B)  // default: DATA_GRANULARITY_64B
#elif (IDEAL_VARIABLE_GRANULARITY == ENABLE)
#define DATA_MANAGEMENT_GRANULARITY         (DATA_GRANULARITY_4096B)
#define DATA_LINE_OFFSET_BITS               (lg2(DATA_GRANULARITY_64B))
#else
#define DATA_MANAGEMENT_GRANULARITY         (DATA_GRANULARITY_64B)
#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_VARIABLE_GRANULARITY

#define DATA_MANAGEMENT_OFFSET_BITS         (lg2(DATA_MANAGEMENT_GRANULARITY))  // data management granularity means how the hardware cluster the data
#define DATA_GRANULARITY_IN_CACHE_LINE      (DATA_MANAGEMENT_GRANULARITY / DATA_GRANULARITY_64B)
#endif // IDEAL_SINGLE_MEMPOD

// CPU setting for branch_predictor (bimodal, gshare, hashed_perceptron, perceptron)
#define BRANCH_USE_BIMODAL             (O3_CPU::bpred_t::bbranchDbimodal)
#define BRANCH_USE_GSHARE              (O3_CPU::bpred_t::bbranchDgshare)
#define BRANCH_USE_HASHED_PERCEPTRON   (O3_CPU::bpred_t::bbranchDhashed_perceptron)
#define BRANCH_USE_PERCEPTRON          (O3_CPU::bpred_t::bbranchDperceptron)
#define BRANCH_PREDICTOR               (BRANCH_USE_BIMODAL)

// CPU setting for branch target buffer (basic_btb)
#define BRANCH_TARGET_BUFFER_USE_BASIC (O3_CPU::btb_t::bbtbDbasic_btb)
#define BRANCH_TARGET_BUFFER           (BRANCH_TARGET_BUFFER_USE_BASIC)

// CPU setting for instruction prefetcher (no_instr, next_line_instr)
#define INSTRUCTION_PREFETCHER_USE_NO_INSTR        (O3_CPU::ipref_t::pprefetcherDno_instr)
#define INSTRUCTION_PREFETCHER_USE_NEXT_LINE_INSTR (O3_CPU::ipref_t::pprefetcherDnext_line_instr)
#define INSTRUCTION_PREFETCHER                     (INSTRUCTION_PREFETCHER_USE_NO_INSTR)

// Cache setting for replacement policy (lru, ship, srrip, drrip)
#define REPLACEMENT_USE_LRU            (CACHE::repl_t::rreplacementDlru)
#define REPLACEMENT_USE_SHIP           (CACHE::repl_t::rreplacementDship)
#define REPLACEMENT_USE_SRRIP          (CACHE::repl_t::rreplacementDsrrip)
#define REPLACEMENT_USE_DRRIP          (CACHE::repl_t::rreplacementDdrrip)
#define LLC_REPLACEMENT_POLICY         (REPLACEMENT_USE_LRU)
// For cpu's private caches:
#define CPU_L2C_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_L1D_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_STLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU_L1I_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_ITLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU_DTLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)

// Cache setting for data prefetcher (no, next_line, ip_stride, no_instr, next_line_instr)
#define PREFETCHER_USE_NO              (CACHE::pref_t::pprefetcherDno)
#define PREFETCHER_USE_NEXT_LINE       (CACHE::pref_t::pprefetcherDnext_line)
#define PREFETCHER_USE_IP_STRIDE       (CACHE::pref_t::pprefetcherDip_stride)
#define PREFETCHER_USE_NO_INSTR        (CACHE::pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
#define PREFETCHER_USE_NEXT_LINE_INSTR (CACHE::pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_)

#define LLC_PREFETCHER                 (PREFETCHER_USE_NO)
#define CPU_L2C_PREFETCHER             (PREFETCHER_USE_NO)
#define CPU_L1D_PREFETCHER             (PREFETCHER_USE_NO)
#define CPU_STLB_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU_ITLB_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU_DTLB_PREFETCHER            (PREFETCHER_USE_NO)
/** @note
 *  Note here you might need to modify the preprocessor "CPU_L1I_PREFETCHER" manually.
 *  For example, if if INSTRUCTION_PREFETCHER == INSTRUCTION_PREFETCHER_USE_NO_INSTR, define CPU_L1I_PREFETCHER = PREFETCHER_USE_NO_INSTR.
 *  Or if INSTRUCTION_PREFETCHER == INSTRUCTION_PREFETCHER_USE_NEXT_LINE_INSTR, define CPU_L1I_PREFETCHER = PREFETCHER_USE_NEXT_LINE_INSTR.
*/
#ifdef INSTRUCTION_PREFETCHER
#define CPU_L1I_PREFETCHER             (PREFETCHER_USE_NO_INSTR)
//#define CPU_L1I_PREFETCHER             (CACHE::pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_)
#endif  // INSTRUCTION_PREFETCHER

#if (USE_OPENMP == ENABLE)
#define SET_THREADS_NUMBER             (6)
#endif  // USE_OPENMP

#define KB (1024ul) // unit is byte
#define MB (KB*KB)
#define GB (MB*KB)

// Standard libraries

/* Includes */
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <cassert>
#include <array>
#include <string>

#if (USE_OPENMP == ENABLE)
#include <omp.h>
#endif  // USE_OPENMP

/* Defines (C++ style) */

// data output class
class DATA_OUTPUT
{
public:
    const std::string data_name;
    FILE* file_handler;
    char* file_name;
    const std::string file_extension;

    DATA_OUTPUT(std::string v1, std::string v2);
    ~DATA_OUTPUT();

    void output_file_initialization(char** string_array, uint32_t number);
};

// memory trace output class
class MEMORY_TRACE: public DATA_OUTPUT
{
public:
    MEMORY_TRACE(std::string v1, std::string v2);
    MEMORY_TRACE(std::string v1, std::string v2, char** string_array, uint32_t number);

    void output_memory_trace_hexadecimal(uint64_t address, char type);
};

// simulator statistics output class
class SIMULATOR_STATISTICS: public DATA_OUTPUT
{
public:
#define PAGE_TABLE_LEVEL_NUMBER     (5u)

    std::array<uint64_t, PAGE_TABLE_LEVEL_NUMBER> valid_pte_count = {0};
    uint64_t virtual_page_count;

    uint64_t read_request_in_memory, read_request_in_memory2;
    uint64_t write_request_in_memory, write_request_in_memory2;

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    uint64_t load_request_in_memory, load_request_in_memory2;
    uint64_t store_request_in_memory, store_request_in_memory2;
#endif // TRACKING_LOAD_STORE_STATISTICS

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
#endif  // IDEAL_VARIABLE_GRANULARITY

    SIMULATOR_STATISTICS(std::string v1, std::string v2);
    SIMULATOR_STATISTICS(std::string v1, std::string v2, char** string_array, uint32_t number);
    ~SIMULATOR_STATISTICS();
};

extern MEMORY_TRACE output_memorytrace;
extern SIMULATOR_STATISTICS output_statistics;

#endif  // USER_CODES

/** @note
 *  600.perlbench_s-210B.champsimtrace.xz
 *  602.gcc_s-734B.champsimtrace.xz
 *  603.bwaves_s-3699B.champsimtrace.xz
 *  605.mcf_s-665B.champsimtrace.xz
 *  607.cactuBSSN_s-2421B.champsimtrace.xz
 *  619.lbm_s-4268B.champsimtrace.xz
 *  620.omnetpp_s-874B.champsimtrace.xz
 *  621.wrf_s-575B.champsimtrace.xz
 *  623.xalancbmk_s-700B.champsimtrace.xz
 *  625.x264_s-18B.champsimtrace.xz
 *  627.cam4_s-573B.champsimtrace.xz
 *  628.pop2_s-17B.champsimtrace.xz
 *  631.deepsjeng_s-928B.champsimtrace.xz has 2000000000 PIN traces
 *  638.imagick_s-10316B.champsimtrace.xz
 *  641.leela_s-800B.champsimtrace.xz
 *  644.nab_s-5853B.champsimtrace.xz
 *  648.exchange2_s-1699B.champsimtrace.xz
 *  649.fotonik3d_s-1176B.champsimtrace.xz
 *  654.roms_s-842B.champsimtrace.xz
 *  657.xz_s-3167B.champsimtrace.xz
 *  Protection: 1989/4/15-1989/6/4, 39'54'12'N 116'23'30'E
 *
 */
#endif

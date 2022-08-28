#ifndef __PROJECTCONFIGURATION_H
#define __PROJECTCONFIGURATION_H

#define ENABLE  (1)
#define DISABLE (0)

#define DEBUG_PRINTF (ENABLE)
#define USER_CODES   (ENABLE)

/* Functionality options */
#if (USER_CODES) == (ENABLE)
#define RAMULATOR                      (ENABLE) // whether use ramulator
#define MEMORY_USE_HYBRID              (ENABLE) // whether use hybrid memory system
#define PRINT_STATISTICS_INTO_FILE     (ENABLE)
#define PRINT_MEMORY_TRACE             (DISABLE)

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
// For cpu0's private caches:
#define CPU0_L2C_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU0_L1D_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU0_STLB_REPLACEMENT_POLICY   (REPLACEMENT_USE_LRU)
#define CPU0_L1I_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU0_ITLB_REPLACEMENT_POLICY   (REPLACEMENT_USE_LRU)
#define CPU0_DTLB_REPLACEMENT_POLICY   (REPLACEMENT_USE_LRU)

// Cache setting for data prefetcher (no, next_line, ip_stride, no_instr, next_line_instr)
#define PREFETCHER_USE_NO              (CACHE::pref_t::pprefetcherDno)
#define PREFETCHER_USE_NEXT_LINE       (CACHE::pref_t::pprefetcherDnext_line)
#define PREFETCHER_USE_IP_STRIDE       (CACHE::pref_t::pprefetcherDip_stride)
#define PREFETCHER_USE_NO_INSTR        (CACHE::pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
#define PREFETCHER_USE_NEXT_LINE_INSTR (CACHE::pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_)

#define LLC_PREFETCHER                 (PREFETCHER_USE_NO)
#define CPU0_L2C_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU0_L1D_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU0_STLB_PREFETCHER           (PREFETCHER_USE_NO)
#define CPU0_ITLB_PREFETCHER           (PREFETCHER_USE_NO)
#define CPU0_DTLB_PREFETCHER           (PREFETCHER_USE_NO)
/** @note
 *  Note here you might need to modify the preprocessor "CPU0_L1I_PREFETCHER" manually.
 *  For example, if if INSTRUCTION_PREFETCHER == INSTRUCTION_PREFETCHER_USE_NO_INSTR, define CPU0_L1I_PREFETCHER = PREFETCHER_USE_NO_INSTR.
 *  Or if INSTRUCTION_PREFETCHER == INSTRUCTION_PREFETCHER_USE_NEXT_LINE_INSTR, define CPU0_L1I_PREFETCHER = PREFETCHER_USE_NEXT_LINE_INSTR.
*/
#ifdef INSTRUCTION_PREFETCHER
#define CPU0_L1I_PREFETCHER            (PREFETCHER_USE_NO_INSTR)
//#define CPU0_L1I_PREFETCHER            (CACHE::pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_)
#endif  // INSTRUCTION_PREFETCHER

#define KB (1024ul) // unit is byte
#define MB (KB*KB)
#define GB (MB*KB)

#if (MEMORY_USE_HYBRID) == (ENABLE)
#define NUMBER_OF_MEMORIES   (2)    // we use two memories for hybrid memory system.
#else
#define NUMBER_OF_MEMORIES   (1)

#endif
// Standard libraries

/* Includes */
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>

/* Defines */
typedef enum
{
    read_default = 0,
    read_end = 1,
} ReadPINTraceType;

typedef struct
{
    uint64_t PIN_trace_number;
    uint64_t total_PIN_trace_number;
    ReadPINTraceType read_PIN_trace_end;
} PINTraceReadType;

typedef struct
{
    FILE* trace_file;
    char* trace_string;
    uint64_t address;    // The address's granularity is byte.
    uint8_t access_type; // read access = 0, write access = 1
} OutputMemoryTraceFileType;

typedef struct
{
    FILE* trace_file;
    char* trace_string;
} OutputChampSimStatisticsFileType;

/* Declaration */

void output_memory_trace_initialization(const char* string);
void output_memory_trace_deinitialization(OutputMemoryTraceFileType& outputmemorytrace);
void output_memory_trace_hexadecimal(OutputMemoryTraceFileType& outputmemorytrace, uint64_t address, char type);

void output_champsim_statistics_initialization(const char* string);
void output_champsim_statistics_deinitialization(OutputChampSimStatisticsFileType& outputchampsimstatistics);

extern OutputMemoryTraceFileType outputmemorytrace_one;
extern OutputChampSimStatisticsFileType outputchampsimstatistics;

#endif  // USER_CODES

/** @note
 * 631.deepsjeng_s-928B.champsimtrace.xz has 2000000000 PIN traces
 *
 */
#endif

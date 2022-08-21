#ifndef __PROJECTCONFIGURATION_H
#define __PROJECTCONFIGURATION_H

#define ENABLE  (1)
#define DISABLE (0)

#define DEBUG_PRINTF (ENABLE)
#define USER_CODES   (ENABLE)

/* Functionality options */
#if USER_CODES == ENABLE
#define PREFETCHER_USE_NO_AND_NO_INSTR (ENABLE)
#define BRANCH_USE_BIMODAL             (ENABLE)
#define REPLACEMENT_USE_LRU            (ENABLE)
#define BTB_USE_BASIC                  (ENABLE)
#define RAMULATOR                      (ENABLE) // whether use ramulator
#define MEMORY_USE_HYBRID              (ENABLE) // whether use hybrid memory system
#define PRINT_STATISTICS_INTO_FILE     (ENABLE)
#define PRINT_MEMORY_TRACE             (ENABLE)


#define KB (1024ul) // unit is byte
#define MB (KB*KB)
#define GB (MB*KB)

#if MEMORY_USE_HYBRID == ENABLE
#define NUMBER_OF_MEMORIES   (2)    // we use two memories for hybrid memory system.
#else
#define NUMBER_OF_MEMORIES   (1)

#endif
// Standard libraries

/* Includes */
#include <stdint.h>
#include <string.h>

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
    uint64_t address;    // full_addr >> 6 = address (granularity: cache line or 64 Byte)
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

#endif

/** @note
 * 631.deepsjeng_s-928B.champsimtrace.xz has 2000000000 PIN traces
 *
 */
#endif

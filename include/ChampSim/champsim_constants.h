#ifndef CHAMPSIM_CONSTANTS_H
#define CHAMPSIM_CONSTANTS_H

/* Header */
#include <cstdlib>

#include "ChampSim/util/bits.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

constexpr unsigned BLOCK_SIZE           = 64; // The unit of BLOCK_SIZE is byte
constexpr unsigned PAGE_SIZE            = 4096;
constexpr uint64_t STAT_PRINTING_PERIOD = 10000000;

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
constexpr std::size_t NUM_CPUS = 2;
#else
constexpr std::size_t NUM_CPUS = 1;
#endif // CPU_USE_MULTIPLE_CORES

constexpr auto LOG2_BLOCK_SIZE           = champsim::lg2(BLOCK_SIZE);
constexpr auto LOG2_PAGE_SIZE            = champsim::lg2(PAGE_SIZE);

/** @note
 *  First, a DRAM chip is the set of all cells on the same silicon die, along with the I/O circuitry which allows
 *  the cells to be accessed from outside the chip.
 *  Second, a DRAM chip is divided into multiple banks (e.g., eight for DDR3), where a bank is a set of cells that
 *  share peripheral circuitry such as the address decoders (row and column).
 *  Third, a bank is further sub-divided into tens of subarrays, where a subarray is a set of cells that share bitlines
 *  and sense-amplifiers.
 *  The main purpose of a sense-amplifier is to read from a cell by reliably detecting the very small amount of
 *  electrical charge stored in the cell. And the set of all sense-amplifiers in a subarray is called a row-buffer.
 *  [reference](https://doi.org/10.1109/HPCA.2013.6522354)
 *
 *  In DDR, we have multiple DRAM chips (devices) on the same PCB. Because a 64-byte cache line is striped across banks
 *  in different DRAM chips to maximize parallelism -> The width of DRAM chip's data bus (Bidirectional) varies according
 *  to the DDR configuration. This means all chips in the same rank will operate together when transferring data, so no need
 *  to set chip address field in physical address.
 *
 *  The unit of DRAM_IO_FREQ is MHz. Here if DRAM_IO_FREQ is 3200ul, the DRAM is DDR4 (800-1600 MHz). DDR5's I/O clock rate ranges
 *  between 2400-3600 MHz, so its data rate can be 4800-7200 MT/s.
*/
constexpr uint64_t DRAM_IO_FREQ          = 3200; // MT/s
constexpr std::size_t DRAM_CHANNELS      = 1;
constexpr std::size_t DRAM_RANKS         = 1;
constexpr std::size_t DRAM_BANKS         = 8;
constexpr std::size_t DRAM_ROWS          = 65536;
constexpr std::size_t DRAM_COLUMNS       = 128;
constexpr std::size_t DRAM_CAPACITY      = 4 * GiB; // 1*GiB / 768*MiB

/** @note
 *  Note if you modify its capacity, its corresponding parameters (e.g., channel, bank, ...) need to be modified too.
 *  if DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 65536, DDR_COLUMNS = 128,
 *  then width of DDR_CHANNELS is 0 bit, width of DDR_RANKS is 0 bit, width of DDR_BANKS is 3 bit,
 *  width of DDR_ROWS is 16 bit, and width of DDR_COLUMNS is 7 bit,
 *  so, the width of physical address is 0 + 0 + 3 + 16 + 7 + 6 (block offset) = 32 bit,
 *  thus, the maximum physical memory capacity is 4 GiB.
 *
 *  Physical address (32 bit for 4 GiB DDR capacity)
 *  | row address | rank index | column address (bank index) | channel | block offset |
 *  |   16 bits   |    0 bit   |           10 bits (3 bits)  |  0 bit  |    6 bits    |
 *  e.g., DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 65536, DDR_COLUMNS = 128 (10 - 3) for 4 GiB DDR capacity.
 *
 *  Physical address (30 bit for (512, 1024] MiB DDR capacity)
 *  | row address | rank index | column address (bank index) | channel | block offset |
 *  |   15 bits   |    0 bit   |            9 bits (3 bits)  |  0 bit  |    6 bits    |
 *  e.g., DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 32768, DDR_COLUMNS = 64 (9 - 3) for (512, 1024] MiB DDR capacity.
 */

constexpr std::size_t DRAM_CHANNEL_WIDTH = 8; // The unit of DRAM_CHANNEL_WIDTH is byte
constexpr std::size_t DRAM_WQ_SIZE       = 64;
constexpr std::size_t DRAM_RQ_SIZE       = 64;

#else
constexpr unsigned BLOCK_SIZE            = 64;
constexpr unsigned PAGE_SIZE             = 4096;
constexpr uint64_t STAT_PRINTING_PERIOD  = 10000000;
constexpr std::size_t NUM_CPUS           = 1;
constexpr auto LOG2_BLOCK_SIZE           = champsim::lg2(BLOCK_SIZE);
constexpr auto LOG2_PAGE_SIZE            = champsim::lg2(PAGE_SIZE);
constexpr uint64_t DRAM_IO_FREQ          = 3200;
constexpr std::size_t DRAM_CHANNELS      = 1;
constexpr std::size_t DRAM_RANKS         = 1;
constexpr std::size_t DRAM_BANKS         = 8;
constexpr std::size_t DRAM_ROWS          = 65536;
constexpr std::size_t DRAM_COLUMNS       = 128;
constexpr std::size_t DRAM_CHANNEL_WIDTH = 8;
constexpr std::size_t DRAM_WQ_SIZE       = 64;
constexpr std::size_t DRAM_RQ_SIZE       = 64;
#endif

#if (USER_CODES == ENABLE)
/** CPU setting for branch_predictor (bimodal, gshare, hashed_perceptron, perceptron) */
#define BRANCH_USE_BIMODAL             (O3_CPU::bbranchDbimodal)
#define BRANCH_USE_GSHARE              (O3_CPU::bbranchDgshare)
#define BRANCH_USE_HASHED_PERCEPTRON   (O3_CPU::bbranchDhashed_perceptron)
#define BRANCH_USE_PERCEPTRON          (O3_CPU::bbranchDperceptron)
#define BRANCH_PREDICTOR               (BRANCH_USE_HASHED_PERCEPTRON)

/** CPU setting for branch target buffer (basic_btb) */
#define BRANCH_TARGET_BUFFER_USE_BASIC (O3_CPU::tbtbDbasic_btb)
#define BRANCH_TARGET_BUFFER           (BRANCH_TARGET_BUFFER_USE_BASIC)

/** Cache setting for replacement policy (drrip, lru, ship, srrip) */
#define REPLACEMENT_USE_DRRIP          (CACHE::rreplacementDdrrip)
#define REPLACEMENT_USE_LRU            (CACHE::rreplacementDlru)
#define REPLACEMENT_USE_SHIP           (CACHE::rreplacementDship)
#define REPLACEMENT_USE_SRRIP          (CACHE::rreplacementDsrrip)

#define LLC_REPLACEMENT_POLICY         (REPLACEMENT_USE_LRU)
// For CPU's private caches:
#define CPU_DTLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU_ITLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)
#define CPU_L1D_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_L1I_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_L2C_REPLACEMENT_POLICY     (REPLACEMENT_USE_LRU)
#define CPU_STLB_REPLACEMENT_POLICY    (REPLACEMENT_USE_LRU)

/** Cache setting for data and instruction prefetchers (ip_stride, next_line, next_line_instr, no, no_instr, spp, va_ampm_lite) */
#define PREFETCHER_USE_IP_STRIDE       (CACHE::pprefetcherDip_stride)
#define PREFETCHER_USE_NEXT_LINE       (CACHE::pprefetcherDnext_line)
#define PREFETCHER_USE_NEXT_LINE_INSTR (CACHE::pprefetcherDnext_line_instr)
#define PREFETCHER_USE_NO              (CACHE::pprefetcherDno)
#define PREFETCHER_USE_NO_INSTR        (CACHE::pprefetcherDno_instr)
#define PREFETCHER_USE_SPP             (CACHE::pprefetcherDspp_dev)
#define PREFETCHER_USE_VA_AMPM_LITE    (CACHE::pprefetcherDva_ampm_lite)

#define LLC_PREFETCHER                 (PREFETCHER_USE_NO)
#define CPU_L2C_PREFETCHER             (PREFETCHER_USE_NO)
#define CPU_L1D_PREFETCHER             (PREFETCHER_USE_NO)
#define CPU_STLB_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU_ITLB_PREFETCHER            (PREFETCHER_USE_NO)
#define CPU_DTLB_PREFETCHER            (PREFETCHER_USE_NO)

#define CPU_L1I_PREFETCHER             (PREFETCHER_USE_NO_INSTR)

/* Virtual memory */
#define PAGE_TABLE_LEVELS              (5ul)
#define MINOR_FAULT_PENALTY            (200ul)

/* CPU */
#define CPU_0                          (0)      // CPU ID
#define CPU_FREQUENCY                  (4000.0) // MHz
#define CPU_DIB_SET                    (32)     // DIB sets
#define CPU_DIB_WAY                    (8)      // DIB ways
#define CPU_DIB_WINDOW                 (16)     // DIB window_size
#define CPU_IFETCH_BUFFER_SIZE         (64)
#define CPU_DECODE_BUFFER_SIZE         (32)
#define CPU_DISPATCH_BUFFER_SIZE       (32)
#define CPU_ROB_SIZE                   (352)
#define CPU_LQ_SIZE                    (128)
#define CPU_SQ_SIZE                    (72)
#define CPU_FETCH_WIDTH                (6)
#define CPU_DECODE_WIDTH               (6)
#define CPU_DISPATCH_WIDTH             (6)
#define CPU_EXECUTE_WIDTH              (4)
#define CPU_LQ_WIDTH                   (2)
#define CPU_SQ_WIDTH                   (2)
#define CPU_RETIRE_WIDTH               (5)
#define CPU_MISPREDICT_PENALTY         (1)
#define CPU_SCHEDULER_SIZE             (128)
#define CPU_DECODE_LATENCY             (1)
#define CPU_DISPATCH_LATENCY           (1)
#define CPU_SCHEDULE_LATENCY           (0)
#define CPU_EXECUTE_LATENCY            (0)
#define CPU_L1I_BANDWIDTH              (1)
#define CPU_L1D_BANDWIDTH              (1)

/* L1I */
#define L1I_CAPACITY                   (32 * KiB) // Default: 32 KiB
#define L1I_WAYS                       (8)
#define L1I_SETS                       (L1I_CAPACITY / BLOCK_SIZE / L1I_WAYS)
#define L1I_PQ_SIZE                    (32)
#define L1I_MSHR_SIZE                  (32)
#define L1I_HIT_LATENCY                (3)
#define L1I_FILL_LATENCY               (1)
#define L1I_TAG_BANDWIDTH              (1)
#define L1I_FILL_BANDWIDTH             (1)

/* L1D */
#define L1D_CAPACITY                   (32 * KiB) // Default: 48 KiB
#define L1D_WAYS                       (8)
#define L1D_SETS                       (L1D_CAPACITY / BLOCK_SIZE / L1D_WAYS)
#define L1D_PQ_SIZE                    (32)
#define L1D_MSHR_SIZE                  (32)
#define L1D_HIT_LATENCY                (4)
#define L1D_FILL_LATENCY               (1)
#define L1D_TAG_BANDWIDTH              (1)
#define L1D_FILL_BANDWIDTH             (1)

/* L2C */
#define L2C_CAPACITY                   (256 * KiB) // Default: 512 KiB
#define L2C_WAYS                       (8)
#define L2C_SETS                       (L2C_CAPACITY / BLOCK_SIZE / L2C_WAYS)
#define L2C_PQ_SIZE                    (32)
#define L2C_MSHR_SIZE                  (64)
#define L2C_HIT_LATENCY                (9)
#define L2C_FILL_LATENCY               (1)
#define L2C_TAG_BANDWIDTH              (1)
#define L2C_FILL_BANDWIDTH             (1)

/* ITLB */
#define ITLB_CAPACITY                  (256 * KiB)
#define ITLB_WAYS                      (4)
#define ITLB_SETS                      (ITLB_CAPACITY / PAGE_SIZE / ITLB_WAYS)
#define ITLB_PQ_SIZE                   (16)
#define ITLB_MSHR_SIZE                 (8)
#define ITLB_HIT_LATENCY               (1)
#define ITLB_FILL_LATENCY              (1)
#define ITLB_TAG_BANDWIDTH             (1)
#define ITLB_FILL_BANDWIDTH            (1)

/* DTLB */
#define DTLB_CAPACITY                  (256 * KiB)
#define DTLB_WAYS                      (4)
#define DTLB_SETS                      (DTLB_CAPACITY / PAGE_SIZE / DTLB_WAYS)
#define DTLB_PQ_SIZE                   (16)
#define DTLB_MSHR_SIZE                 (8)
#define DTLB_HIT_LATENCY               (1)
#define DTLB_FILL_LATENCY              (1)
#define DTLB_TAG_BANDWIDTH             (1)
#define DTLB_FILL_BANDWIDTH            (1)

/* STLB */
#define STLB_CAPACITY                  (6 * MiB)
#define STLB_WAYS                      (12)
#define STLB_SETS                      (STLB_CAPACITY / PAGE_SIZE / STLB_WAYS)
#define STLB_PQ_SIZE                   (32)
#define STLB_MSHR_SIZE                 (16)
#define STLB_HIT_LATENCY               (7)
#define STLB_FILL_LATENCY              (1)
#define STLB_TAG_BANDWIDTH             (1)
#define STLB_FILL_BANDWIDTH            (1)

/* LLC */
#define LLC_CAPACITY                   (1 * MiB) // Default: 2 MiB
#define LLC_WAYS                       (16)
#define LLC_SETS                       (LLC_CAPACITY / BLOCK_SIZE / LLC_WAYS)
#define LLC_PQ_SIZE                    (32)
#define LLC_MSHR_SIZE                  (64)
#define LLC_HIT_LATENCY                (19)
#define LLC_FILL_LATENCY               (1)
#define LLC_TAG_BANDWIDTH              (NUM_CPUS)
#define LLC_FILL_BANDWIDTH             (NUM_CPUS)

/* PTW */
#define PTW_MSHR_SIZE                  (5)

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
#define CPU_1 (1) // CPU ID

#endif // CPU_USE_MULTIPLE_CORES

// Clock scale
#if (RAMULATOR == ENABLE)
#define MEMORY_CONTROLLER_CLOCK_SCALE (1.0)
#else
#define MEMORY_CONTROLLER_CLOCK_SCALE (CPU_FREQUENCY / DRAM_IO_FREQ) // 4000 MHz / 3200 MHz = 1.25
#endif

#define CACHE_CLOCK_SCALE           (1.0)
#define O3_CPU_CLOCK_SCALE          (1.0)
#define PAGETABLEWALKER_CLOCK_SCALE (1.0)

/** @note
 *  The connection of each module, e.g., cpu, cache, TLB, PTW, memory controller.
 *  cpu  -> ITLB, DTLB, L1I, L1D
 *  DTLB -> STLB
 *  ITLB -> STLB
 *  L1I  -> L2C
 *  STLB -> PTW
 *  PTW  -> L1D
 *  L1D  -> L2C
 *  L2C  -> LLC
 *  LLC  -> memory_controller
*/

#endif // USER_CODES

#endif

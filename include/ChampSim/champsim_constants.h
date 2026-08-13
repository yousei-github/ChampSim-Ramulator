#ifndef CHAMPSIM_CONSTANTS_H
#define CHAMPSIM_CONSTANTS_H

/* Header */

#include <cstdlib>

#include "ChampSim/util/bits.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

namespace champsim
{
#ifdef DEBUG_PRINT
constexpr bool debug_print = true;
#else
constexpr bool debug_print = false;
#endif /* DEBUG_PRINT */

} // namespace champsim

// ChampSim build hash ID, based on commit ID 24cc41bb94bac4da7ffa18774524529bb0e57c44 (2024-12)
constexpr unsigned long long CHAMPSIM_BUILD = 0x11a7870cb042c20b;

// Number of CPUs (num_cores)
#if (CPU_USE_MULTIPLE_CORES == ENABLE)
constexpr std::size_t NUM_CPUS = 2;
#else
constexpr std::size_t NUM_CPUS = 1;
#endif /* CPU_USE_MULTIPLE_CORES */

// Cache block (or cache line) size (block_size)
constexpr unsigned BLOCK_SIZE                = 64; // The unit is byte
// Page size (page_size)
constexpr unsigned PAGE_SIZE                 = 4096;

constexpr unsigned LOG2_BLOCK_SIZE           = champsim::lg2(BLOCK_SIZE);
constexpr unsigned LOG2_PAGE_SIZE            = champsim::lg2(PAGE_SIZE);

constexpr long long STAT_PRINTING_PERIOD     = 10000000; // heartbeat_frequency

/**
 * @note
 * The basic idea of building large DRAM arrays while minimizing access latency is to divide the memory into smaller arrays and interconnect these arrays to input/output buses.
 * This method creates a hierarchical structure for the large DRAM array.
 * When the memory controller accesses DRAM, it follows a specific sequence: Channel → Rank → Chip → Bank Group → Bank → Array.
 * A monolithic DRAM array is divided into multiple banks that can be accessed independently, either during the same cycle or in consecutive cycles.
 * This design reduces access latency for the memory array and allows for multiple simultaneous accesses, a process known as interleaving or banking.
 *
 * Channel is the top of the DRAM subsystem hierarchy from the memory controller's view, representing a unit of true independence.
 * Each channel serves as an independent pathway that enables full parallelism.
 * For DDR4, each channel typically features a 64-bit data bus (or 72 bits with Error-Correcting Code).
 *
 * DIMM refers to the packaging of memory, not its architecture.
 *
 * Rank is the set of devices that are selected together by a single Chip Select (CS#) signal pin.
 * All chips in a rank receive the same command, as well as the same bank (and bank group), row, and column addresses.
 * The only pins that are private to each chip are the DQ (data bus) and DQS (data strobe) pins.
 * The chips within a rank operate in lockstep, meaning they perform the same set of operations simultaneously in parallel, with each chip contributing to a portion of the data bus.
 * This is why configurations such as 8x8, 16x4, and 4x16 all result in a total data bus width of 64 bits (where 'x8' indicates that a chip has an 8-bit-wide data interface).
 *
 * Chip (device) refers to the packaged DRAM die where density and organization come together.
 * Chips come in different data bus widths, including x4, x8, and x16.
 * Because all chips in the same rank will operate together when transferring data, there is no need to set chip address field in physical address.
 *
 * Bank group refers to the collection of banks that have separate prefetch resources.
 * The DRAM core (i.e., cell array core) has stopped scaling in accordance with the I/O clock due to physical frequency limitations.
 * To address this issue, memory manufacturers have implemented bank groups.
 * This allows the chip to alternate between groups, enabling a continuous stream of data.
 *
 * Bank is the unit of concurrency within a chip/rank.
 * Each bank is an independent 2D array with its own row buffer (sense amplifier), row decoder, column decoder, and state machine.
 *
 * Row is one word line within a bank.
 * 
 * Column refers to the address within the open row buffer that selects the specific slice of data to be sent out over the data bus (DQ).
 * With DDR4's 8n prefetch, one column command transfers 8 times (BL8, which corresponds to 8 beats) the device width per chip over 4 clocks (or 8 I/O cycles).
 * This means that 8 × 64 bits are transmitted across a rank, totaling 64 bytes, which is precisely one cache line.
 * 
 * [reference](https://safari.ethz.ch/ddca/spring2026/lib/exe/fetch.php?media=onur-ddca-2026-lecture21-memory-organization-hierarchy-afterlecture.pdf)
 * [reference](https://doi.org/10.1109/HPCA.2013.6522354)
 *
 * DDR memory transfers data twice per clock cycle (hence "double data rate"), once on the rising edge and once on the falling edge of the clock signal,
 * effectively doubling the data transfer rate compared to single data rate (SDR) memory.
 *
 * DDR IO rate (or I/O clock) refers to the rate at which data is transferred on the input/output (IO) pins of a memory module.
 * It's essentially the speed at which data moves between the memory and the processor. This rate is determined by both the memory clock frequency and the technology generation (DDR, DDR2, DDR3, etc.).
 * Note memory clock is the base clock frequency of the memory chip itself. 
 * 
 * For example, a DDR3-1600 module has a memory clock of 200 MHz and an I/O clock of 800 MHz, resulting in a data transfer rate of 1600 MT/s (Mega transfers per second).
*/
constexpr uint32_t DRAM_DATA_TRANSFER_RATE   = 3200;                                                // MT/s (Data transfer rate)
constexpr uint32_t ONE_SECOND_IN_MILLISECOND = 1000ul;                                              // Millisecond
constexpr uint32_t ONE_SECOND_IN_MICROSECOND = ONE_SECOND_IN_MILLISECOND * 1000ul;                  // Microsecond
constexpr uint32_t DRAM_DATA_TRANSFER_PERIOD = ONE_SECOND_IN_MICROSECOND / DRAM_DATA_TRANSFER_RATE; // Picosecond
constexpr uint32_t DRAM_IO_FREQ              = DRAM_DATA_TRANSFER_RATE / 2;                         // MH/z
constexpr uint32_t DRAM_IO_CLOCK_PERIOD      = ONE_SECOND_IN_MICROSECOND / DRAM_IO_FREQ;            // Picosecond

constexpr std::size_t DRAM_CHANNEL_WIDTH     = 8; // The unit of DRAM_CHANNEL_WIDTH is byte
constexpr std::size_t DRAM_CHANNELS          = 1;
constexpr std::size_t DRAM_RANKS             = 1;
constexpr std::size_t DRAM_BANK_GROUPS       = 8;
constexpr std::size_t DRAM_BANKS             = 4;
constexpr std::size_t DRAM_ROWS              = 65536;
constexpr std::size_t DRAM_COLUMNS           = 1024;
constexpr std::size_t DRAM_CAPACITY          = DRAM_CHANNEL_WIDTH * DRAM_CHANNELS * DRAM_RANKS * DRAM_RANKS * DRAM_BANK_GROUPS * DRAM_BANKS * DRAM_ROWS * DRAM_COLUMNS; // The unit of DRAM_CAPACITY is byte

constexpr std::size_t DRAM_RQ_SIZE           = 64;
constexpr std::size_t DRAM_WQ_SIZE           = 64;

/**
 * DRAM timings for ChampSim's own memory controller (used when both RAMULATOR and RAMULATOR2 are * disabled).
 * The unit of tRP/tRCD/tCAS/tRAS is DRAM I/O cycles;
 * Ramulator reads its own timings from its .cfg / .yaml configuration instead.
 */
constexpr uint32_t DRAM_tRP                  = 24;                             // The time of precharging phase
constexpr uint32_t DRAM_tRCD                 = 24;                             // Row-to-column delay
constexpr uint32_t DRAM_tCAS                 = 24;                             // The time between sending a column address to the memory and the beginning of the data in response
constexpr uint32_t DRAM_tRAS                 = 52;                             // Row active to precharge delay
constexpr uint32_t DRAM_REFRESH_PERIOD       = 32 * ONE_SECOND_IN_MILLISECOND; // Microsecond (32 ms)
constexpr uint32_t DRAM_REFRESHES_PER_PERIOD = 8192;

#else
constexpr unsigned BLOCK_SIZE            = 64;
constexpr unsigned PAGE_SIZE             = 4096;
constexpr long long STAT_PRINTING_PERIOD = 10000000;
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
#endif /* USER_CODES */

#if (USER_CODES == ENABLE)
/* Virtual memory */
#define PAGE_TABLE_LEVELS              (5ul)
#define MINOR_FAULT_PENALTY            (CPU_CLOCK_PERIOD * 200ul)
#define VMEM_RANDOMIZATION             (1) // Seed for randomized virtual-to-physical page mapping

/**
 * Cache setting for replacement policy (drrip, lru, ship, srrip)
 * @see include/ChampSim/replacement/
 */
#define REPLACEMENT_USE_DRRIP          drrip
#define REPLACEMENT_USE_LRU            lru
#define REPLACEMENT_USE_SHIP           ship
#define REPLACEMENT_USE_SRRIP          srrip

// For CPU's private caches:
#define CPU_DTLB_REPLACEMENT_POLICY    REPLACEMENT_USE_LRU
#define CPU_ITLB_REPLACEMENT_POLICY    REPLACEMENT_USE_LRU
#define CPU_L1D_REPLACEMENT_POLICY     REPLACEMENT_USE_LRU
#define CPU_L1I_REPLACEMENT_POLICY     REPLACEMENT_USE_LRU
#define CPU_L2C_REPLACEMENT_POLICY     REPLACEMENT_USE_LRU
#define CPU_STLB_REPLACEMENT_POLICY    REPLACEMENT_USE_LRU
// For last level cache:
#define LLC_REPLACEMENT_POLICY         REPLACEMENT_USE_LRU

/**
 * Cache setting for data and instruction prefetchers (ip_stride, next_line, no, no_instr, spp, va_ampm_lite)
 * @see include/ChampSim/prefetcher/
 */
#define PREFETCHER_USE_IP_STRIDE       ip_stride
#define PREFETCHER_USE_NEXT_LINE       next_line
#define PREFETCHER_USE_NO              no
#define PREFETCHER_USE_SPP             spp_dev
#define PREFETCHER_USE_VA_AMPM_LITE    va_ampm_lite

// For CPU's private caches:
#define CPU_DTLB_PREFETCHER            PREFETCHER_USE_NO
#define CPU_ITLB_PREFETCHER            PREFETCHER_USE_NO
#define CPU_L1D_PREFETCHER             PREFETCHER_USE_NO
#define CPU_L1I_PREFETCHER             PREFETCHER_USE_NO
#define CPU_L2C_PREFETCHER             PREFETCHER_USE_NO
#define CPU_STLB_PREFETCHER            PREFETCHER_USE_NO
// For last level cache:
#define LLC_PREFETCHER                 PREFETCHER_USE_NO

/**
 * CPU setting for branch predictor (bimodal, gshare, hashed_perceptron, perceptron)
 * @see include/ChampSim/branch/
 */
#define BRANCH_USE_BIMODAL             bimodal
#define BRANCH_USE_GSHARE              gshare
#define BRANCH_USE_HASHED_PERCEPTRON   hashed_perceptron
#define BRANCH_USE_PERCEPTRON          perceptron
#define CPU_BRANCH_PREDICTOR           BRANCH_USE_BIMODAL

/**
 * CPU setting for branch target buffer (basic_btb)
 * @see include/ChampSim/btb/
 */
#define BRANCH_TARGET_BUFFER_USE_BASIC basic_btb
#define CPU_BRANCH_TARGET_BUFFER       BRANCH_TARGET_BUFFER_USE_BASIC

/* CPUs' IDs */
#if (CPU_USE_MULTIPLE_CORES == DISABLE)
#define CPU_0 (0) // CPU 0 ID

#else
#define CPU_0 (0) // CPU 0 ID
#define CPU_1 (1) // CPU 1 ID

#endif /* CPU_USE_MULTIPLE_CORES */

#define CPU_FREQUENCY            (4000.0)                                                 // MHz (ooo_cpu's frequency)
#define CPU_CLOCK_PERIOD         ((uint32_t) (ONE_SECOND_IN_MICROSECOND / CPU_FREQUENCY)) // Picosecond

#define CPU_DIB_SET              (32)  // DIB (Decoded Instruction Buffer) sets (sets)
#define CPU_DIB_WAY              (8)   // DIB ways (ways)
#define CPU_DIB_WINDOW           (16)  // DIB window size (window_size)
#define CPU_IFETCH_BUFFER_SIZE   (64)  // (ifetch_buffer_size)
#define CPU_DECODE_BUFFER_SIZE   (32)  // (decode_buffer_size)
#define CPU_DISPATCH_BUFFER_SIZE (32)  // (dispatch_buffer_size)
#define CPU_DIB_HIT_BUFFER_SIZE  (32)  // DIB hit buffer size
#define CPU_REGISTER_FILE_SIZE   (128) // (register_file_size)
#define CPU_ROB_SIZE             (352) // (rob_size)
#define CPU_LQ_SIZE              (128) // (lq_size)
#define CPU_SQ_SIZE              (72)  // (sq_size)
#define CPU_FETCH_WIDTH          (6)   // (fetch_width)
#define CPU_DECODE_WIDTH         (6)   // (decode_width)
#define CPU_DISPATCH_WIDTH       (6)   // (dispatch_width)
#define CPU_EXECUTE_WIDTH        (4)   // (execute_width)
#define CPU_LQ_WIDTH             (2)   // (lq_width)
#define CPU_SQ_WIDTH             (2)   // (sq_width)
#define CPU_RETIRE_WIDTH         (5)   // (retire_width)
#define CPU_DIB_INORDER_WIDTH    (5)   // DIB inorder width
#define CPU_MISPREDICT_PENALTY   (1)   // (mispredict_penalty)
#define CPU_SCHEDULER_SIZE       (128) // (scheduler_size)
#define CPU_DECODE_LATENCY       (1)   // (decode_latency)
#define CPU_DIB_HIT_LATENCY      (1)   // DIB hit latency
#define CPU_DISPATCH_LATENCY     (1)   // (dispatch_latency)
#define CPU_SCHEDULE_LATENCY     (0)   // (schedule_latency)
#define CPU_EXECUTE_LATENCY      (0)   // (execute_latency)
#define CPU_L1I_BANDWIDTH        (1)
#define CPU_L1D_BANDWIDTH        (1)

/** @todo Delete unused macro and add champsim_config.json setting to here */

/**
 * Per-cache geometry, latency, and bandwidth.
 * The *_LATENCY values are total latencies in cycles; cache_builder splits each one into a fill
 * latency of (latency + 1) / 2 and a hit latency of the remainder, and clamps the total to a
 * minimum of 2 cycles (so the TLBs' latency of 1 is effectively 2, as upstream).
 * *_MAX_TAG_CHECK and *_MAX_FILL are the per-cycle tag-check and fill bandwidths.
 */

/* L1I */
#define L1I_CAPACITY             (32 * KiB)                             // Default: 32 KiB
#define L1I_WAYS                 (8)                                    // (ways)
#define L1I_SETS                 (L1I_CAPACITY / BLOCK_SIZE / L1I_WAYS) // (sets)
#define L1I_RQ_SIZE              (64)
#define L1I_WQ_SIZE              (64)
#define L1I_PQ_SIZE              (32)
#define L1I_MSHR_SIZE            (8)
#define L1I_LATENCY              (4)
#define L1I_MAX_TAG_CHECK        (2)
#define L1I_MAX_FILL             (2)

static_assert((L1I_CAPACITY / BLOCK_SIZE) % L1I_WAYS == 0, "L1I Capacity is not enough");

/* L1D */
#define L1D_CAPACITY      (48 * KiB) // Default: 48 KiB
#define L1D_WAYS          (12)
#define L1D_SETS          (L1D_CAPACITY / BLOCK_SIZE / L1D_WAYS)
#define L1D_RQ_SIZE       (64)
#define L1D_WQ_SIZE       (64)
#define L1D_PQ_SIZE       (8)
#define L1D_MSHR_SIZE     (16)
#define L1D_LATENCY       (5)
#define L1D_MAX_TAG_CHECK (2)
#define L1D_MAX_FILL      (2)

static_assert((L1D_CAPACITY / BLOCK_SIZE) % L1D_WAYS == 0, "L1D Capacity is not enough");

/* L2C */
#define L2C_CAPACITY      (512 * KiB) // Default: 512 KiB
#define L2C_WAYS          (8)
#define L2C_SETS          (L2C_CAPACITY / BLOCK_SIZE / L2C_WAYS)
#define L2C_RQ_SIZE       (32)
#define L2C_WQ_SIZE       (32)
#define L2C_PQ_SIZE       (16)
#define L2C_MSHR_SIZE     (32)
#define L2C_LATENCY       (10)
#define L2C_MAX_TAG_CHECK (1)
#define L2C_MAX_FILL      (1)

static_assert((L2C_CAPACITY / BLOCK_SIZE) % L2C_WAYS == 0, "L2C Capacity is not enough");

/* ITLB */
#define ITLB_CAPACITY      (256 * KiB)
#define ITLB_WAYS          (4)
#define ITLB_SETS          (ITLB_CAPACITY / PAGE_SIZE / ITLB_WAYS)
#define ITLB_RQ_SIZE       (16)
#define ITLB_WQ_SIZE       (16)
#define ITLB_PQ_SIZE       (0)
#define ITLB_MSHR_SIZE     (8)
#define ITLB_LATENCY       (1)
#define ITLB_MAX_TAG_CHECK (2)
#define ITLB_MAX_FILL      (2)

static_assert((ITLB_CAPACITY / PAGE_SIZE) % ITLB_WAYS == 0, "ITLB Capacity is not enough");

/* DTLB */
#define DTLB_CAPACITY      (256 * KiB)
#define DTLB_WAYS          (4)
#define DTLB_SETS          (DTLB_CAPACITY / PAGE_SIZE / DTLB_WAYS)
#define DTLB_RQ_SIZE       (16)
#define DTLB_WQ_SIZE       (16)
#define DTLB_PQ_SIZE       (0)
#define DTLB_MSHR_SIZE     (8)
#define DTLB_LATENCY       (1)
#define DTLB_MAX_TAG_CHECK (2)
#define DTLB_MAX_FILL      (2)

static_assert((DTLB_CAPACITY / PAGE_SIZE) % DTLB_WAYS == 0, "DTLB Capacity is not enough");

/* STLB */
#define STLB_CAPACITY      (6 * MiB)
#define STLB_WAYS          (12)
#define STLB_SETS          (STLB_CAPACITY / PAGE_SIZE / STLB_WAYS)
#define STLB_RQ_SIZE       (32)
#define STLB_WQ_SIZE       (32)
#define STLB_PQ_SIZE       (0)
#define STLB_MSHR_SIZE     (16)
#define STLB_LATENCY       (8)
#define STLB_MAX_TAG_CHECK (1)
#define STLB_MAX_FILL      (1)

static_assert((STLB_CAPACITY / PAGE_SIZE) % STLB_WAYS == 0, "STLB Capacity is not enough");

/* LLC */
#define LLC_CAPACITY      (2 * MiB) // Default: 2 MiB
#define LLC_WAYS          (16)
#define LLC_SETS          (LLC_CAPACITY / BLOCK_SIZE / LLC_WAYS)
#define LLC_RQ_SIZE       (32)
#define LLC_WQ_SIZE       (32)
#define LLC_PQ_SIZE       (32)
#define LLC_MSHR_SIZE     (64)
#define LLC_LATENCY       (20)
#define LLC_MAX_TAG_CHECK (1)
#define LLC_MAX_FILL      (1)

static_assert((LLC_CAPACITY / BLOCK_SIZE) % LLC_WAYS == 0, "LLC Capacity is not enough");

/* PTW */
#define PTW_RQ_SIZE                  (16)
#define PTW_MSHR_SIZE                (5)
#define PTW_PSCL5_SET                (1)
#define PTW_PSCL5_WAY                (2)
#define PTW_PSCL4_SET                (1)
#define PTW_PSCL4_WAY                (4)
#define PTW_PSCL3_SET                (2)
#define PTW_PSCL3_WAY                (4)
#define PTW_PSCL2_SET                (4)
#define PTW_PSCL2_WAY                (8)

/**
 * Cache and PTW channel (inter-component queue) sizes — shared across all CPUs.
 * These size the champsim::channel queues constructed in include/ChampSim/core_inst.h.
 * Each channel is sized by its *lower* endpoint's queue sizes, so the macros below are aliases
 * of the per-cache *_RQ_SIZE / *_PQ_SIZE / *_WQ_SIZE macros above rather than independent knobs.
 */
/* From CPU pipeline */
#define CPU_to_L1I_CHANNEL_RQ_SIZE   L1I_RQ_SIZE
#define CPU_to_L1I_CHANNEL_PQ_SIZE   L1I_PQ_SIZE
#define CPU_to_L1I_CHANNEL_WQ_SIZE   L1I_WQ_SIZE
#define CPU_to_L1D_CHANNEL_RQ_SIZE   L1D_RQ_SIZE
#define CPU_to_L1D_CHANNEL_PQ_SIZE   L1D_PQ_SIZE
#define CPU_to_L1D_CHANNEL_WQ_SIZE   L1D_WQ_SIZE

/* From PTW */
#define PTW_to_L1D_CHANNEL_RQ_SIZE   L1D_RQ_SIZE
#define PTW_to_L1D_CHANNEL_PQ_SIZE   L1D_PQ_SIZE
#define PTW_to_L1D_CHANNEL_WQ_SIZE   L1D_WQ_SIZE

/* L1 → Translation lookaside buffer */
#define L1D_to_DTLB_CHANNEL_RQ_SIZE  DTLB_RQ_SIZE
#define L1D_to_DTLB_CHANNEL_PQ_SIZE  DTLB_PQ_SIZE
#define L1D_to_DTLB_CHANNEL_WQ_SIZE  DTLB_WQ_SIZE
#define L1I_to_ITLB_CHANNEL_RQ_SIZE  ITLB_RQ_SIZE
#define L1I_to_ITLB_CHANNEL_PQ_SIZE  ITLB_PQ_SIZE
#define L1I_to_ITLB_CHANNEL_WQ_SIZE  ITLB_WQ_SIZE

/* L1 → L2 */
#define L1D_to_L2C_CHANNEL_RQ_SIZE   L2C_RQ_SIZE
#define L1D_to_L2C_CHANNEL_PQ_SIZE   L2C_PQ_SIZE
#define L1D_to_L2C_CHANNEL_WQ_SIZE   L2C_WQ_SIZE
#define L1I_to_L2C_CHANNEL_RQ_SIZE   L2C_RQ_SIZE
#define L1I_to_L2C_CHANNEL_PQ_SIZE   L2C_PQ_SIZE
#define L1I_to_L2C_CHANNEL_WQ_SIZE   L2C_WQ_SIZE

/* L1/L2 TLB → STLB */
#define DTLB_to_STLB_CHANNEL_RQ_SIZE STLB_RQ_SIZE
#define DTLB_to_STLB_CHANNEL_PQ_SIZE STLB_PQ_SIZE
#define DTLB_to_STLB_CHANNEL_WQ_SIZE STLB_WQ_SIZE
#define ITLB_to_STLB_CHANNEL_RQ_SIZE STLB_RQ_SIZE
#define ITLB_to_STLB_CHANNEL_PQ_SIZE STLB_PQ_SIZE
#define ITLB_to_STLB_CHANNEL_WQ_SIZE STLB_WQ_SIZE
#define L2C_to_STLB_CHANNEL_RQ_SIZE  STLB_RQ_SIZE
#define L2C_to_STLB_CHANNEL_PQ_SIZE  STLB_PQ_SIZE
#define L2C_to_STLB_CHANNEL_WQ_SIZE  STLB_WQ_SIZE

/* STLB → PTW (read-only path: pq and wq are 0 by design) */
#define STLB_to_PTW_CHANNEL_RQ_SIZE  PTW_RQ_SIZE
#define STLB_to_PTW_CHANNEL_PQ_SIZE  (0)
#define STLB_to_PTW_CHANNEL_WQ_SIZE  (0)

/* L2 → LLC */
#define L2C_to_LLC_CHANNEL_RQ_SIZE   LLC_RQ_SIZE
#define L2C_to_LLC_CHANNEL_PQ_SIZE   LLC_PQ_SIZE
#define L2C_to_LLC_CHANNEL_WQ_SIZE   LLC_WQ_SIZE

/** @todo [deprecated] */
// Clock scale
#if (RAMULATOR == ENABLE) || (RAMULATOR2 == ENABLE)
#define MEMORY_CONTROLLER_CLOCK_SCALE (1.0)
#else
#define MEMORY_CONTROLLER_CLOCK_SCALE (CPU_FREQUENCY / DRAM_DATA_TRANSFER_RATE) // 4000 MHz / 3200 MHz = 1.25

#endif /* RAMULATOR */

#define CACHE_CLOCK_SCALE           (1.0)
#define O3_CPU_CLOCK_SCALE          (1.0)
#define PAGETABLEWALKER_CLOCK_SCALE (1.0)

/**
 * @note
 * The connection of each module, e.g., cpu, cache, TLB, PTW, memory controller.
 * cpu  -> ITLB, DTLB, L1I, L1D
 * DTLB -> STLB
 * ITLB -> STLB
 * L1I  -> L2C
 * STLB -> PTW
 * PTW  -> L1D
 * L1D  -> L2C
 * L2C  -> LLC
 * LLC  -> memory_controller
*/

#endif /* USER_CODES */

#endif

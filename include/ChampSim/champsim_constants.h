#ifndef CHAMPSIM_CONSTANTS_H
#define CHAMPSIM_CONSTANTS_H
#include <cstdlib>

#include "ChampSim/util/bits.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

constexpr unsigned BLOCK_SIZE           = 64; // The unit of BLOCK_SIZE is byte
constexpr unsigned PAGE_SIZE            = 4096;
constexpr uint64_t STAT_PRINTING_PERIOD = 10000000;

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
constexpr std::size_t NUM_CPUS      = 2;
constexpr std::size_t NUM_CACHES    = NUM_CPUS * 6u + 1u;
constexpr std::size_t NUM_OPERABLES = NUM_CACHES + NUM_CPUS * 2u + 1u;
#else
constexpr std::size_t NUM_CPUS      = 1;
constexpr std::size_t NUM_CACHES    = 7;
constexpr std::size_t NUM_OPERABLES = 10;
#endif // CPU_USE_MULTIPLE_CORES

constexpr auto LOG2_BLOCK_SIZE                     = champsim::lg2(BLOCK_SIZE);
constexpr auto LOG2_PAGE_SIZE                      = champsim::lg2(PAGE_SIZE);

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
constexpr uint64_t DRAM_IO_FREQ                    = 3200; // MT/s
constexpr std::size_t DRAM_CHANNELS                = 1;
constexpr std::size_t DRAM_RANKS                   = 1;
constexpr std::size_t DRAM_BANKS                   = 8;
constexpr std::size_t DRAM_ROWS                    = 65536;
constexpr std::size_t DRAM_COLUMNS                 = 128;
constexpr std::size_t DRAM_CAPACITY                = 4 * GiB; // 1*GiB / 768*MiB

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

/** @note
 *  Below are parameters for 3D-stacked DRAM (or High Bandwidth Memory (HBM)).
 *  HBM uses the same IO frequency as DRAM.
*/

constexpr std::size_t HBM_CHANNELS                 = 8;
constexpr std::size_t HBM_BANKS                    = 8;
constexpr std::size_t HBM_ROWS                     = 1024;
constexpr std::size_t HBM_COLUMNS                  = 64;
constexpr std::size_t HBM_CAPACITY                 = 0 * MiB; // 0*MiB / 256*MiB

/** @note
 *  Physical address (30 bit for 1 GiB HBM capacity)
 *  | row address | rank index | column address (bank index) | channel | block offset |
 *  |   11 bits   |    0 bit   |           10 bits (3 bits)  |  3 bits |    6 bits    |
 *  e.g., HBM_CHANNELS = 8, HBM_BANKS = 8, HBM_ROWS = 2048, HBM_COLUMNS = 128 (10 - 3) for 1 GiB HBM capacity.
 *
 *  Physical address (28 bit for 256 MiB HBM capacity)
 *  | row address | rank index | column address (bank index) | channel | block offset |
 *  |   10 bits   |    0 bit   |            9 bits (3 bits)  |  3 bits |    6 bits    |
 *  e.g., HBM_CHANNELS = 8, HBM_BANKS = 8, HBM_ROWS = 1024, HBM_COLUMNS = 64 (9 - 3) for 256 MiB HBM capacity.
 */

constexpr std::size_t DRAM_CHANNEL_WIDTH           = 8; // The unit of DRAM_CHANNEL_WIDTH is byte
constexpr std::size_t DRAM_WQ_SIZE                 = 64;
constexpr std::size_t DRAM_RQ_SIZE                 = 64;
constexpr std::size_t tRP_DRAM_NANOSECONDS         = 12.5;
constexpr std::size_t tRCD_DRAM_NANOSECONDS        = 12.5;
constexpr std::size_t tCAS_DRAM_NANOSECONDS        = 12.5;
constexpr std::size_t DBUS_TURN_AROUND_NANOSECONDS = 7.5;

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
/* Virtual memory */
#if (MEMORY_USE_HYBRID == ENABLE)
#define MEMORY_CAPACITY (HBM_CAPACITY + DDR_CAPACITY)
#else
#define MEMORY_CAPACITY (DDR_CAPACITY)
#endif // MEMORY_USE_HYBRID
#define PAGE_TABLE_LEVELS        (5ul)
#define MINOR_FAULT_PENALTY      (200ul)

/* LLC */
#define LLC_CAPACITY             (1 * MiB) // Default: 2 MiB
#define LLC_WAYS                 (16)
#define LLC_SETS                 (LLC_CAPACITY / BLOCK_SIZE / LLC_WAYS)
#define LLC_WQ_SIZE              (32)
#define LLC_RQ_SIZE              (32)
#define LLC_PQ_SIZE              (32)
#define LLC_MSHR_SIZE            (64)
#define LLC_LATENCY              (20)
#define LLC_FILL_LATENCY         (1)
#define LLC_MAX_READ             (1)
#define LLC_MAX_WRITE            (1)
#define LLC_PREFETCH_AS_LOAD     (false) // true/false
#define LLC_WQ_FULL_ADDRESS      (false) // true/false
#define LLC_VIRTUAL_PREFETCH     (false) // true/false
#define LLC_PREF_ACTIVATE_MASK   (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L2C */
#define L2C_CAPACITY             (256 * KiB) // Default: 512 KiB
#define L2C_WAYS                 (8)
#define L2C_SETS                 (L2C_CAPACITY / BLOCK_SIZE / L2C_WAYS)
#define L2C_WQ_SIZE              (32)
#define L2C_RQ_SIZE              (32)
#define L2C_PQ_SIZE              (16)
#define L2C_MSHR_SIZE            (32)
#define L2C_LATENCY              (10)
#define L2C_FILL_LATENCY         (1)
#define L2C_MAX_READ             (1)
#define L2C_MAX_WRITE            (1)
#define L2C_PREFETCH_AS_LOAD     (false) // true/false
#define L2C_WQ_FULL_ADDRESS      (false) // true/false
#define L2C_VIRTUAL_PREFETCH     (false) // true/false
#define L2C_PREF_ACTIVATE_MASK   (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L1D */
#define L1D_CAPACITY             (32 * KiB) // Default: 48 KiB
#define L1D_WAYS                 (8)        // Default: 12
#define L1D_SETS                 (L1D_CAPACITY / BLOCK_SIZE / L1D_WAYS)
#define L1D_WQ_SIZE              (64)
#define L1D_RQ_SIZE              (64)
#define L1D_PQ_SIZE              (8)
#define L1D_MSHR_SIZE            (16)
#define L1D_LATENCY              (4) // Default: 5
#define L1D_FILL_LATENCY         (1)
#define L1D_MAX_READ             (2)
#define L1D_MAX_WRITE            (2)
#define L1D_PREFETCH_AS_LOAD     (false) // true/false
#define L1D_WQ_FULL_ADDRESS      (true)  // true/false
#define L1D_VIRTUAL_PREFETCH     (false) // true/false
#define L1D_PREF_ACTIVATE_MASK   (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* PTW */
#define PTW_CPU                  (CPU_0)
#define PTW_PSCL5_SET            (1)
#define PTW_PSCL5_WAY            (2)
#define PTW_PSCL4_SET            (1)
#define PTW_PSCL4_WAY            (4)
#define PTW_PSCL3_SET            (2)
#define PTW_PSCL3_WAY            (4)
#define PTW_PSCL2_SET            (4)
#define PTW_PSCL2_WAY            (8)
#define PTW_RQ_SIZE              (16)
#define PTW_MSHR_SIZE            (5)
#define PTW_MAX_READ             (2)
#define PTW_MAX_WRITE            (2)
#define PTW_LATENCY              (1)

/* STLB */
#define STLB_SETS                (128)
#define STLB_WAYS                (12)
#define STLB_WQ_SIZE             (32)
#define STLB_RQ_SIZE             (32)
#define STLB_PQ_SIZE             (0)
#define STLB_MSHR_SIZE           (16)
#define STLB_LATENCY             (8)
#define STLB_FILL_LATENCY        (1)
#define STLB_MAX_READ            (1)
#define STLB_MAX_WRITE           (1)
#define STLB_PREFETCH_AS_LOAD    (false) // true/false
#define STLB_WQ_FULL_ADDRESS     (false) // true/false
#define STLB_VIRTUAL_PREFETCH    (false) // true/false
#define STLB_PREF_ACTIVATE_MASK  (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L1I */
#define L1I_CAPACITY             (32 * KiB) // Default: 32 KiB
#define L1I_WAYS                 (8)
#define L1I_SETS                 (L1I_CAPACITY / BLOCK_SIZE / L1I_WAYS)
#define L1I_WQ_SIZE              (64)
#define L1I_RQ_SIZE              (64)
#define L1I_PQ_SIZE              (32)
#define L1I_MSHR_SIZE            (8)
#define L1I_LATENCY              (4)
#define L1I_FILL_LATENCY         (1)
#define L1I_MAX_READ             (2)
#define L1I_MAX_WRITE            (2)
#define L1I_PREFETCH_AS_LOAD     (false) // true/false
#define L1I_WQ_FULL_ADDRESS      (true)  // true/false
#define L1I_VIRTUAL_PREFETCH     (true)  // true/false
#define L1I_PREF_ACTIVATE_MASK   (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* ITLB */
#define ITLB_SETS                (16)
#define ITLB_WAYS                (4)
#define ITLB_WQ_SIZE             (16)
#define ITLB_RQ_SIZE             (16)
#define ITLB_PQ_SIZE             (0)
#define ITLB_MSHR_SIZE           (8)
#define ITLB_LATENCY             (1)
#define ITLB_FILL_LATENCY        (1)
#define ITLB_MAX_READ            (2)
#define ITLB_MAX_WRITE           (2)
#define ITLB_PREFETCH_AS_LOAD    (false) // true/false
#define ITLB_WQ_FULL_ADDRESS     (true)  // true/false
#define ITLB_VIRTUAL_PREFETCH    (true)  // true/false
#define ITLB_PREF_ACTIVATE_MASK  (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* DTLB */
#define DTLB_SETS                (16)
#define DTLB_WAYS                (4)
#define DTLB_WQ_SIZE             (16)
#define DTLB_RQ_SIZE             (16)
#define DTLB_PQ_SIZE             (0)
#define DTLB_MSHR_SIZE           (8)
#define DTLB_LATENCY             (1)
#define DTLB_FILL_LATENCY        (1)
#define DTLB_MAX_READ            (2)
#define DTLB_MAX_WRITE           (2)
#define DTLB_PREFETCH_AS_LOAD    (false) // true/false
#define DTLB_WQ_FULL_ADDRESS     (true)  // true/false
#define DTLB_VIRTUAL_PREFETCH    (false) // true/false
#define DTLB_PREF_ACTIVATE_MASK  (5)     // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* CPU */
#define CPU_0                    (0)      // CPU number
#define CPU_FREQUENCY            (4000.0) // MHz
#define CPU_DIB_SET              (32)     // DIB sets
#define CPU_DIB_WAY              (8)      // DIB ways
#define CPU_DIB_WINDOW           (16)     // DIB window_size
#define CPU_IFETCH_BUFFER_SIZE   (64)
#define CPU_DECODE_BUFFER_SIZE   (32)
#define CPU_DISPATCH_BUFFER_SIZE (32)
#define CPU_ROB_SIZE             (352)
#define CPU_LQ_SIZE              (128)
#define CPU_SQ_SIZE              (72)
#define CPU_FETCH_WIDTH          (6)
#define CPU_DECODE_WIDTH         (6)
#define CPU_DISPATCH_WIDTH       (6)
#define CPU_SCHEDULER_SIZE       (128)
#define CPU_EXECUTE_WIDTH        (4)
#define CPU_LQ_WIDTH             (2)
#define CPU_SQ_WIDTH             (2)
#define CPU_RETIRE_WIDTH         (5)
#define CPU_MISPREDICT_PENALTY   (1)
#define CPU_DECODE_LATENCY       (1)
#define CPU_DISPATCH_LATENCY     (1)
#define CPU_SCHEDULE_LATENCY     (0)
#define CPU_EXECUTE_LATENCY      (0)

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
#define CPU_1 (1) // CPU number
#endif            // CPU_USE_MULTIPLE_CORES

// Clock scale
#if (RAMULATOR == ENABLE)
#define MEMORY_CONTROLLER_CLOCK_SCALE (1.0)
#else
#define MEMORY_CONTROLLER_CLOCK_SCALE (CPU_FREQUENCY / DRAM_IO_FREQ) // 4000 MHz / 3200 MHz = 1.25
#endif

#define CACHE_CLOCK_SCALE           (1.0)
#define O3_CPU_CLOCK_SCALE          (1.0)
#define PAGETABLEWALKER_CLOCK_SCALE (1.0)

// Memory request consumer level
#define DTLB_LEVEL                  (1)
#define ITLB_LEVEL                  (1)
#define L1I_LEVEL                   (1)
#define STLB_LEVEL                  (2)
#define PTW_LEVEL                   (3)
#define L1D_LEVEL                   (4)
#define L2C_LEVEL                   (5)
#define LLC_LEVEL                   (6)

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
 *
*/

#endif // USER_CODES

#endif

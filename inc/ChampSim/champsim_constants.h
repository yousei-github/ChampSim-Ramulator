#ifndef CHAMPSIM_CONSTANTS_H
#define CHAMPSIM_CONSTANTS_H
#include "util.h"
#include "ProjectConfiguration.h" // user file

#if (USER_CODES == ENABLE)
#define BLOCK_SIZE           (64ul)  // the unit of BLOCK_SIZE is byte
#define LOG2_BLOCK_SIZE      (lg2(BLOCK_SIZE))
#define PAGE_SIZE            (4096ul)
#define LOG2_PAGE_SIZE       (lg2(PAGE_SIZE))
#define STAT_PRINTING_PERIOD (10000000ul)
#define NUM_CPUS             (1u)
#define NUM_CACHES           (7u)
#define NUM_OPERABLES        (10u)

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
#define DRAM_IO_FREQ      (3200ul)
#define DRAM_IO_DATA_RATE (3200ul)  // MT/s

#define DDR_CHANNELS (1ul)
#define DDR_RANKS    (1ul)
#define DDR_BANKS    (8ul)
#define DDR_ROWS     (65536ul)
#define DDR_COLUMNS  (128ul)
#define DDR_CAPACITY (4*GB) // 1*GB / 768*MB
 /** @note
  *  Note if you modify its capacity, its corresponding parameters (e.g., channel, bank, ...) need to be modified too.
  *  if DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 65536, DDR_COLUMNS = 128,
  *  then width of DDR_CHANNELS is 0 bit, width of DDR_RANKS is 0 bit, width of DDR_BANKS is 3 bit,
  *  width of DDR_ROWS is 16 bit, and width of DDR_COLUMNS is 7 bit,
  *  so, the width of physical address is 0 + 0 + 3 + 16 + 7 + 6 (block offset) = 32 bit,
  *  thus, the maximum physical memory capacity is 4 GB.
  *
  *  Physical address (32 bit for 4 GB DDR capacity)
  *  | row address | rank index | column address (bank index) | channel | block offset |
  *  |   16 bits   |    0 bit   |           10 bits (3 bits)  |  0 bit  |    6 bits    |
  *  e.g., DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 65536, DDR_COLUMNS = 128 (10 - 3) for 4 GB DDR capacity.
  *
  *  Physical address (30 bit for (512, 1024] MB DDR capacity)
  *  | row address | rank index | column address (bank index) | channel | block offset |
  *  |   15 bits   |    0 bit   |            9 bits (3 bits)  |  0 bit  |    6 bits    |
  *  e.g., DDR_CHANNELS = 1, DDR_RANKS = 1, DDR_BANKS = 8, DDR_ROWS = 32768, DDR_COLUMNS = 64 (9 - 3) for (512, 1024] MB DDR capacity.
 */

 /** @note
  *  Below are parameters for 3D-stacked DRAM (or High Bandwidth Memory (HBM)).
  *  HBM uses the same IO frequency as DRAM.
 */
#define HBM_CHANNELS (8ul)
#define HBM_BANKS    (8ul)
#define HBM_ROWS     (1024ul)
#define HBM_COLUMNS  (64ul)
#define HBM_CAPACITY (0*MB) // 0*MB / 256*MB
 /** @note
  *  Physical address (30 bit for 1 GB HBM capacity)
  *  | row address | rank index | column address (bank index) | channel | block offset |
  *  |   11 bits   |    0 bit   |           10 bits (3 bits)  |  3 bits |    6 bits    |
  *  e.g., HBM_CHANNELS = 8, HBM_BANKS = 8, HBM_ROWS = 2048, HBM_COLUMNS = 128 (10 - 3) for 1 GB HBM capacity.
  *
  *  Physical address (28 bit for 256 MB HBM capacity)
  *  | row address | rank index | column address (bank index) | channel | block offset |
  *  |   10 bits   |    0 bit   |            9 bits (3 bits)  |  3 bits |    6 bits    |
  *  e.g., HBM_CHANNELS = 8, HBM_BANKS = 8, HBM_ROWS = 1024, HBM_COLUMNS = 64 (9 - 3) for 256 MB HBM capacity.
 */

 // The unit of DRAM_CHANNEL_WIDTH is byte
#define DRAM_CHANNEL_WIDTH           (8ul)
#define DRAM_WQ_SIZE                 (64ul)
#define DRAM_RQ_SIZE                 (64ul)
#define tRP_DRAM_NANOSECONDS         (12.5)
#define tRCD_DRAM_NANOSECONDS        (12.5)
#define tCAS_DRAM_NANOSECONDS        (12.5)
#define DBUS_TURN_AROUND_NANOSECONDS (7.5)

#else
#define BLOCK_SIZE      (64ul)
#define LOG2_BLOCK_SIZE (lg2(BLOCK_SIZE))
#define PAGE_SIZE       (4096ul)
#define LOG2_PAGE_SIZE  (lg2(PAGE_SIZE))
#define STAT_PRINTING_PERIOD (10000000ul)
#define NUM_CPUS        (1ul)
#define NUM_CACHES      (7u)
#define NUM_OPERABLES   (10u)

#define DRAM_IO_FREQ 3200ul
#define DRAM_CHANNELS 1ul
#define DRAM_RANKS 1ul
#define DRAM_BANKS 8ul
#define DRAM_ROWS 65536ul
#define DRAM_COLUMNS 128ul

#define DRAM_CHANNEL_WIDTH           (8ul)
#define DRAM_WQ_SIZE                 (64ul)
#define DRAM_RQ_SIZE                 (64ul)
#define tRP_DRAM_NANOSECONDS         (12.5)
#define tRCD_DRAM_NANOSECONDS        (12.5)
#define tCAS_DRAM_NANOSECONDS        (12.5)
#define DBUS_TURN_AROUND_NANOSECONDS (7.5)
#endif

#if (USER_CODES == ENABLE)
/* Virtual memory */
#if (MEMORY_USE_HYBRID == ENABLE)
#define MEMORY_CAPACITY (HBM_CAPACITY + DDR_CAPACITY)
#else
#define MEMORY_CAPACITY       (DDR_CAPACITY)
#endif  // MEMORY_USE_HYBRID
#define PAGE_TABLE_LEVELS     (5ul)
#define MINOR_FAULT_PENALTY   (200ul)

/* LLC */
#define LLC_SETS                    (2048)
#define LLC_WAYS                    (16)
#define LLC_WQ_SIZE                 (32)
#define LLC_RQ_SIZE                 (32)
#define LLC_PQ_SIZE                 (32)
#define LLC_MSHR_SIZE               (64)
#define LLC_LATENCY                 (20)
#define LLC_FILL_LATENCY            (1)
#define LLC_MAX_READ                (1)
#define LLC_MAX_WRITE               (1)
#define LLC_PREFETCH_AS_LOAD        (false) // true/false
#define LLC_WQ_FULL_ADDRESS         (false) // true/false
#define LLC_VIRTUAL_PREFETCH        (false) // true/false
#define LLC_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L2C */
#define L2C_SETS                    (1024)
#define L2C_WAYS                    (8)
#define L2C_WQ_SIZE                 (32)
#define L2C_RQ_SIZE                 (32)
#define L2C_PQ_SIZE                 (16)
#define L2C_MSHR_SIZE               (32)
#define L2C_LATENCY                 (10)
#define L2C_FILL_LATENCY            (1)
#define L2C_MAX_READ                (1)
#define L2C_MAX_WRITE               (1)
#define L2C_PREFETCH_AS_LOAD        (false) // true/false
#define L2C_WQ_FULL_ADDRESS         (false) // true/false
#define L2C_VIRTUAL_PREFETCH        (false) // true/false
#define L2C_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L1D */
#define L1D_SETS                    (64)
#define L1D_WAYS                    (12)
#define L1D_WQ_SIZE                 (64)
#define L1D_RQ_SIZE                 (64)
#define L1D_PQ_SIZE                 (8)
#define L1D_MSHR_SIZE               (16)
#define L1D_LATENCY                 (5)
#define L1D_FILL_LATENCY            (1)
#define L1D_MAX_READ                (2)
#define L1D_MAX_WRITE               (2)
#define L1D_PREFETCH_AS_LOAD        (false) // true/false
#define L1D_WQ_FULL_ADDRESS         (true) // true/false
#define L1D_VIRTUAL_PREFETCH        (false) // true/false
#define L1D_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* PTW */
#define PTW_CPU                     (CPU_0)
#define PTW_PSCL5_SET               (1)
#define PTW_PSCL5_WAY               (2)
#define PTW_PSCL4_SET               (1)
#define PTW_PSCL4_WAY               (4)
#define PTW_PSCL3_SET               (2)
#define PTW_PSCL3_WAY               (4)
#define PTW_PSCL2_SET               (4)
#define PTW_PSCL2_WAY               (8)
#define PTW_RQ_SIZE                 (16)
#define PTW_MSHR_SIZE               (5)
#define PTW_MAX_READ                (2)
#define PTW_MAX_WRITE               (2)
#define PTW_LATENCY                 (1)

/* STLB */
#define STLB_SETS                    (128)
#define STLB_WAYS                    (12)
#define STLB_WQ_SIZE                 (32)
#define STLB_RQ_SIZE                 (32)
#define STLB_PQ_SIZE                 (0)
#define STLB_MSHR_SIZE               (16)
#define STLB_LATENCY                 (8)
#define STLB_FILL_LATENCY            (1)
#define STLB_MAX_READ                (1)
#define STLB_MAX_WRITE               (1)
#define STLB_PREFETCH_AS_LOAD        (false) // true/false
#define STLB_WQ_FULL_ADDRESS         (false) // true/false
#define STLB_VIRTUAL_PREFETCH        (false) // true/false
#define STLB_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* L1I */
#define L1I_SETS                    (64)
#define L1I_WAYS                    (8)
#define L1I_WQ_SIZE                 (64)
#define L1I_RQ_SIZE                 (64)
#define L1I_PQ_SIZE                 (32)
#define L1I_MSHR_SIZE               (8)
#define L1I_LATENCY                 (4)
#define L1I_FILL_LATENCY            (1)
#define L1I_MAX_READ                (2)
#define L1I_MAX_WRITE               (2)
#define L1I_PREFETCH_AS_LOAD        (false) // true/false
#define L1I_WQ_FULL_ADDRESS         (true) // true/false
#define L1I_VIRTUAL_PREFETCH        (true) // true/false
#define L1I_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* ITLB */
#define ITLB_SETS                    (16)
#define ITLB_WAYS                    (4)
#define ITLB_WQ_SIZE                 (16)
#define ITLB_RQ_SIZE                 (16)
#define ITLB_PQ_SIZE                 (0)
#define ITLB_MSHR_SIZE               (8)
#define ITLB_LATENCY                 (1)
#define ITLB_FILL_LATENCY            (1)
#define ITLB_MAX_READ                (2)
#define ITLB_MAX_WRITE               (2)
#define ITLB_PREFETCH_AS_LOAD        (false) // true/false
#define ITLB_WQ_FULL_ADDRESS         (true) // true/false
#define ITLB_VIRTUAL_PREFETCH        (true) // true/false
#define ITLB_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* DTLB */
#define DTLB_SETS                    (16)
#define DTLB_WAYS                    (4)
#define DTLB_WQ_SIZE                 (16)
#define DTLB_RQ_SIZE                 (16)
#define DTLB_PQ_SIZE                 (0)
#define DTLB_MSHR_SIZE               (8)
#define DTLB_LATENCY                 (1)
#define DTLB_FILL_LATENCY            (1)
#define DTLB_MAX_READ                (2)
#define DTLB_MAX_WRITE               (2)
#define DTLB_PREFETCH_AS_LOAD        (false) // true/false
#define DTLB_WQ_FULL_ADDRESS         (true) // true/false
#define DTLB_VIRTUAL_PREFETCH        (false) // true/false
#define DTLB_PREF_ACTIVATE_MASK      (5) // "LOAD,PREFETCH" = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH))

/* CPU */
#define CPU_0                       (0)      // CPU number
#define CPU_FREQUENCY               (4000.0) // MHz
#define CPU_DIB_SET                 (32)     // DIB sets
#define CPU_DIB_WAY                 (8)      // DIB ways
#define CPU_DIB_WINDOW              (16)     // DIB window_size
#define CPU_IFETCH_BUFFER_SIZE      (64)
#define CPU_DECODE_BUFFER_SIZE      (32)
#define CPU_DISPATCH_BUFFER_SIZE    (32)
#define CPU_ROB_SIZE                (352)
#define CPU_LQ_SIZE                 (128)
#define CPU_SQ_SIZE                 (72)
#define CPU_FETCH_WIDTH             (6)
#define CPU_DECODE_WIDTH            (6)
#define CPU_DISPATCH_WIDTH          (6)
#define CPU_SCHEDULER_SIZE          (128)
#define CPU_EXECUTE_WIDTH           (4)
#define CPU_LQ_WIDTH                (2)
#define CPU_SQ_WIDTH                (2)
#define CPU_RETIRE_WIDTH            (5)
#define CPU_MISPREDICT_PENALTY      (1)
#define CPU_DECODE_LATENCY          (1)
#define CPU_DISPATCH_LATENCY        (1)
#define CPU_SCHEDULE_LATENCY        (0)
#define CPU_EXECUTE_LATENCY         (0)



// Clock scale
#define MEMORY_CONTROLLER_CLOCK_SCALE (CPU_FREQUENCY / DRAM_IO_FREQ)  // 4000 MHz / 3200 MHz = 1.25
#define CACHE_CLOCK_SCALE             (1.0)
#define O3_CPU_CLOCK_SCALE            (1.0)
#define PAGETABLEWALKER_CLOCK_SCALE   (1.0)

// Memory request consumer level
#define DTLB_LEVEL (1)
#define ITLB_LEVEL (1)
#define L1I_LEVEL  (1)
#define STLB_LEVEL (2)
#define PTW_LEVEL  (3)
#define L1D_LEVEL  (4)
#define L2C_LEVEL  (5)
#define LLC_LEVEL  (6)

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

#endif  // USER_CODES

#endif

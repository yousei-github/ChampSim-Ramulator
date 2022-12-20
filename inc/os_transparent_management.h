#ifndef OS_TRANSPARENT_MANAGEMENT_H
#define OS_TRANSPARENT_MANAGEMENT_H
#include <map>
#include <vector>
#include <iostream>
#include <cassert>
#include <deque>

#include "champsim_constants.h"
#include "memory_class.h"
#include "util.h"
#include "ProjectConfiguration.h" // user file

/** @note Abbreviation:
 *  FM -> Fast memory (e.g., HBM, DDR4)
 *  SM -> Slow memory (e.g., DDR4, PCM)
 *
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
#define COUNTER_WIDTH                   uint8_t
#define COUNTER_MAX_VALUE               (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE           (0)

#define HOTNESS_WIDTH                   bool
#define HOTNESS_DEFAULT_VALUE           (false)

#define REMAPPING_LOCATION_WIDTH            uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS       (3)
#define LOCATION_TABLE_ENTRY_WIDTH          uint16_t

// 0x0538 for the congruence group with 5 members (lines) at most, (000_001_010_011_100_0 = 0x0538)
// [15:13] bit for member 0, [12:10] bit for member 1, [9:7] bit for member 2, [6:4] bit for member 3, [3:1] bit for member 4.
#define LOCATION_TABLE_ENTRY_DEFAULT_VALUE      (0x0538)
#define LOCATION_TABLE_ENTRY_MSB                (UINT16_WIDTH - 1)  // MSB -> most significant bit

#define REMAPPING_REQUEST_QUEUE_LENGTH          (64)  // 1024/4096
#define QUEUE_BUSY_DEGREE_THRESHOLD             (0.8f)

#define INCOMPLETE_READ_REQUEST_QUEUE_LENGTH    (128)
#define INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH   (128)
#elif (IDEAL_MULTIPLE_GRANULARITY == ENABLE)
#define COUNTER_WIDTH                       uint8_t
#define COUNTER_MAX_VALUE                   (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE               (0)

#define HOTNESS_WIDTH                       bool
#define HOTNESS_DEFAULT_VALUE               (false)

#define REMAPPING_LOCATION_WIDTH            uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS       (3)

#define START_ADDRESS_WIDTH                 uint8_t
#define START_ADDRESS_WIDTH_BITS            (6)

#define MIGRATION_GRANULARITY_WIDTH         uint8_t
#define MIGRATION_GRANULARITY_WIDTH_BITS    (3)

#define NUMBER_OF_BLOCK                     (5)

#define REMAPPING_REQUEST_QUEUE_LENGTH      (64)  // 1024/4096
#define QUEUE_BUSY_DEGREE_THRESHOLD         (0.8f)

#define INTERVAL_FOR_DECREMENT              (1000000)   // 1000000

#else
#define COUNTER_WIDTH                   uint8_t
#define COUNTER_MAX_VALUE               (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE           (0)

#define HOTNESS_WIDTH                   bool
#define HOTNESS_DEFAULT_VALUE           (false)

#define REMAPPING_LOCATION_WIDTH            uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS       (3)

#endif  // IDEAL_MULTIPLE_GRANULARITY

class OS_TRANSPARENT_MANAGEMENT
{
public:
    uint64_t cycle = 0;
    COUNTER_WIDTH hotness_threshold = 0;
    uint64_t total_capacity;        // uint is byte
    uint64_t fast_memory_capacity;  // uint is byte
    uint64_t total_capacity_at_data_block_granularity;
    uint64_t fast_memory_capacity_at_data_block_granularity;
    uint8_t  fast_memory_offset_bit;    // address format in the data management granularity

    std::vector<COUNTER_WIDTH>& counter_table;      // A counter for every data block
    std::vector<HOTNESS_WIDTH>& hotness_table;      // A hotness bit for every data block, true -> data block is hot, false -> data block is cold.

    /* Remapping request */
    struct RemappingRequest
    {
        uint64_t address_in_fm, address_in_sm;  // hardware address in fast and slow memories
        REMAPPING_LOCATION_WIDTH fm_location, sm_location;
        uint8_t size;   // number of cache lines to remap
    };
    std::deque<RemappingRequest> remapping_request_queue;
    uint64_t remapping_request_queue_congestion;

#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    // scoped enumerations
    enum class RemappingLocation: REMAPPING_LOCATION_WIDTH
    {
        Zero = 0, One, Two, Three, Four,
        Max
    };

    uint8_t  congruence_group_msb;      // most significant bit of congruence group, and its address format is in the byte granularity

    /* Remapping table */
    std::vector<LOCATION_TABLE_ENTRY_WIDTH>& line_location_table;    // paper CAMEO: SRAM-Based LLT / Embed LLT in Stacked DRAM
#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    /** @brief
     *  if a memory read request is mapped in slow memory, the memory controller needs first access the fast memory
     *  to get the Location Entry and Data (LEAD), and then access the slow memory based on that LEAD.
    */
    struct ReadRequest
    {
        PACKET packet;
        bool fm_access_finish = false;    // whether the access in fast memory is completed.
    };
    std::vector<ReadRequest> incomplete_read_request_queue;

    /** @brief
     *  if a memory write request is received, the memory controller needs first to figure out where is the right
     *  place to write. So, the memory controller first access the fast memory to get the Location Entry and Data (LEAD),
     *  and then write the memory (fast or slow memory).
    */
    struct WriteRequest
    {
        PACKET packet;
        bool fm_access_finish = false;    // whether the access in fast memory is completed.
    };
    std::vector<WriteRequest> incomplete_write_request_queue;
#endif  // COLOCATED_LINE_LOCATION_TABLE

#if (IDEAL_MULTIPLE_GRANULARITY == ENABLE)
    // scoped enumerations
    /** @brief
     *  it is used to store the migrated page's block number, which is to represent the location in a set.
    */
    enum class RemappingLocation: REMAPPING_LOCATION_WIDTH
    {
        Zero = 0, One, Two, Three, Four,
        Max = NUMBER_OF_BLOCK
    };

    uint8_t  set_msb;      // most significant bit of set, and its address format is in the byte granularity

    /** @brief
     *  it is used to store the first address of the migrated part of the migrated pages.
    */
    enum class StartAddress: START_ADDRESS_WIDTH
    {
        Zero = 0, One, Two, Three, Four, /* Five ~ Sixtythree */
        Max = 64
    };

    /** @brief
     *  it is used to store the migrated page's migration granularity.
    */
    enum class MigrationGranularity: MIGRATION_GRANULARITY_WIDTH
    {
        None = 0, Byte_64 = 1, Byte_128 = 2, Byte_256 = 4, Byte_512 = 8,
        KiB_1 = 16, KiB_2 = 32, KiB_4 = 64,
        Max
    };

    struct AccessDistribution
    {
        bool access[START_ADDRESS_WIDTH(StartAddress::Max)];

        AccessDistribution()
        {
            for (uint8_t i = 0; i < START_ADDRESS_WIDTH(StartAddress::Max); i++)
            {
                access[i] = false;
            }
        };
    };
    /* Access distribution table */
    std::vector<AccessDistribution>& access_table;  // A access distribution for every data block

    struct PlacementEntry
    {
        REMAPPING_LOCATION_WIDTH cursor; // this cursor is used to track the position of next available group for migration in the placement entry.
        REMAPPING_LOCATION_WIDTH tag[NUMBER_OF_BLOCK];
        START_ADDRESS_WIDTH start_address[NUMBER_OF_BLOCK];
        MIGRATION_GRANULARITY_WIDTH granularity[NUMBER_OF_BLOCK];

        PlacementEntry()
        {
            cursor = 0;

            for (uint8_t i = 0; i < NUMBER_OF_BLOCK; i++)
            {
                tag[i] = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);
                start_address[i] = START_ADDRESS_WIDTH(StartAddress::Zero);
                granularity[i] = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
            }
        };
    };
    /* Remapping table */
    std::vector<PlacementEntry>& placement_table;
#endif  // IDEAL_MULTIPLE_GRANULARITY

    /* Member functions */
    OS_TRANSPARENT_MANAGEMENT(COUNTER_WIDTH threshold, uint64_t max_address, uint64_t fast_memory_max_address);
    ~OS_TRANSPARENT_MANAGEMENT();

    // address is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree);

    // translate the physical address to hardware address
    void physical_to_hardware_address(PACKET& packet);
    void physical_to_hardware_address(uint64_t& address);

    bool issue_remapping_request(RemappingRequest& remapping_request);
    bool finish_remapping_request();

    // detect cold data block
    void cold_data_detection();

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool finish_fm_access_in_incomplete_read_request_queue(uint64_t h_address);
    bool finish_fm_access_in_incomplete_write_request_queue(uint64_t h_address);
#endif  // COLOCATED_LINE_LOCATION_TABLE

private:
    // evict cold data block
    bool cold_data_eviction(uint64_t source_address, float queue_busy_degree);

    // add new remapping request into the remapping_request_queue
    bool enqueue_remapping_request(RemappingRequest& remapping_request);

#if (IDEAL_MULTIPLE_GRANULARITY == ENABLE)
    // calculate the migration granularity based on start_address and end_address.
    MIGRATION_GRANULARITY_WIDTH calculate_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address);

    // check whether this migration granularity is beyond the block's range and adjust it to a proper value, this function returns updated end_address
    START_ADDRESS_WIDTH adjust_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity);
    // check whether this migration granularity is beyond the block's end address and adjust it to a proper value, this function returns updated end_address
    START_ADDRESS_WIDTH round_down_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity);
#endif  // IDEAL_MULTIPLE_GRANULARITY

};

#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif  // OS_TRANSPARENT_MANAGEMENT_H

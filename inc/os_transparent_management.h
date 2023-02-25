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

/* Includes for research */
#include "cameo.h"
#include "variable_granularity.h"
#include "ideal_single_mempod.h"

/** @note Abbreviation:
 *  FM -> Fast memory (e.g., HBM, DDR4)
 *  SM -> Slow memory (e.g., DDR4, PCM)
 *
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (NO_METHOD_FOR_RUN_HYBRID_MEMORY == ENABLE)
#define COUNTER_WIDTH                   uint8_t
#define COUNTER_MAX_VALUE               (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE           (0)

#define HOTNESS_WIDTH                   bool
#define HOTNESS_DEFAULT_VALUE           (false)

#define REMAPPING_LOCATION_WIDTH            uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS       (3)

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

private:
    // evict cold data block
    bool cold_data_eviction(uint64_t source_address, float queue_busy_degree);

    // add new remapping request into the remapping_request_queue
    bool enqueue_remapping_request(RemappingRequest& remapping_request);

};

#endif  // NO_METHOD_FOR_RUN_HYBRID_MEMORY

#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif  // OS_TRANSPARENT_MANAGEMENT_H

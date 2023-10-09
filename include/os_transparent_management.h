#ifndef OS_TRANSPARENT_MANAGEMENT_H
#define OS_TRANSPARENT_MANAGEMENT_H
#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <vector>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/util/bits.h"
#include "ProjectConfiguration.h" // User file

/* Includes for research */
#include "cameo.h"
#include "ideal_single_mempod.h"
#include "variable_granularity.h"

/** @note Abbreviation:
 *  FM -> Fast memory (e.g., HBM, DDR4)
 *  SM -> Slow memory (e.g., DDR4, PCM)
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (NO_METHOD_FOR_RUN_HYBRID_MEMORY == ENABLE)
#define COUNTER_WIDTH                 uint8_t
#define COUNTER_MAX_VALUE             (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE         (0)

#define HOTNESS_WIDTH                 bool
#define HOTNESS_DEFAULT_VALUE         (false)

#define REMAPPING_LOCATION_WIDTH      uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS (3)

class OS_TRANSPARENT_MANAGEMENT
{
    using channel_type = champsim::channel;
    using request_type = typename channel_type::request_type;

public:
    uint64_t cycle                  = 0;
    COUNTER_WIDTH hotness_threshold = 0;
    uint64_t total_capacity;       // Uint is byte
    uint64_t fast_memory_capacity; // Uint is byte
    uint64_t total_capacity_at_data_block_granularity;
    uint64_t fast_memory_capacity_at_data_block_granularity;
    uint8_t fast_memory_offset_bit; // Address format in the data management granularity

    std::vector<COUNTER_WIDTH>& counter_table; // A counter for every data block
    std::vector<HOTNESS_WIDTH>& hotness_table; // A hotness bit for every data block, true -> data block is hot, false -> data block is cold.

    /* Remapping request */
    struct RemappingRequest
    {
        uint64_t address_in_fm, address_in_sm; // Hardware address in fast and slow memories
        REMAPPING_LOCATION_WIDTH fm_location, sm_location;
        uint8_t size; // Number of cache lines to remap
    };

    std::deque<RemappingRequest> remapping_request_queue;
    uint64_t remapping_request_queue_congestion;

    /* Member functions */
    OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address);
    ~OS_TRANSPARENT_MANAGEMENT();

    // Adress is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree);

    // Translate the physical address to hardware address
    void physical_to_hardware_address(request_type& packet);
    void physical_to_hardware_address(uint64_t& address);

    bool issue_remapping_request(RemappingRequest& remapping_request);
    bool finish_remapping_request();

    // Detect cold data block
    void cold_data_detection();

private:
    // Evict cold data block
    bool cold_data_eviction(uint64_t source_address, float queue_busy_degree);

    // Add new remapping request into the remapping_request_queue
    bool enqueue_remapping_request(RemappingRequest& remapping_request);
};

#endif // NO_METHOD_FOR_RUN_HYBRID_MEMORY

#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif // OS_TRANSPARENT_MANAGEMENT_H

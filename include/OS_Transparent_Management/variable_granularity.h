#ifndef VARIABLE_GRANULARITY_H
#define VARIABLE_GRANULARITY_H

#include <cassert>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <vector>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/channel.h"
#include "ChampSim/util/bits.h"
#include "OS_Transparent_Management/os_transparent_management_common.h"
#include "ProjectConfiguration.h" // User file

/**
 * @note Abbreviation:
 * FM -> Fast memory (e.g., HBM, DDR4)
 * SM -> Slow memory (e.g., DDR4, PCM)
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_VARIABLE_GRANULARITY == ENABLE)
#define REMAPPING_LOCATION_WIDTH_SIGN    int8_t // Default: int8_t
#define REMAPPING_LOCATION_WIDTH_BITS    (champsim::lg2(64))

#define START_ADDRESS_WIDTH              uint8_t
#define START_ADDRESS_WIDTH_BITS         (6)

#define MIGRATION_GRANULARITY_WIDTH      uint8_t
#define MIGRATION_GRANULARITY_WIDTH_BITS (3)

#define NUMBER_OF_BLOCK                  (5) // Default: 5

#define REMAPPING_REQUEST_QUEUE_LENGTH   (64) // 1024/4096

#define INTERVAL_FOR_DECREMENT           (1000000) // Default: 1000000

class OS_TRANSPARENT_MANAGEMENT : public OS_TRANSPARENT_MANAGEMENT_BASE
{
    using channel_type = champsim::channel;
    using request_type = typename channel_type::request_type;

public:
    COUNTER_WIDTH hotness_threshold = 0;

    std::vector<COUNTER_WIDTH>& counter_table; // A counter for every data block
    std::vector<HOTNESS_WIDTH>& hotness_table; // A hotness bit for every data block, true -> data block is hot, false -> data block is cold.

    // Scoped enumerations
    /**
     * @brief
     * It is used to store the migrated page's block number, which is to represent the location in a set.
     */
    enum class RemappingLocation : REMAPPING_LOCATION_WIDTH
    {
        Zero = 0,
        One,
        Two,
        Three,
        Four,
        Max = NUMBER_OF_BLOCK
    };

    uint8_t congruence_group_msb; // Most significant bit of the congruence group (set), and its address format is in the byte granularity

    /**
     * @brief
     * It is used to store the first address of the migrated part of the migrated pages.
     */
    enum class StartAddress : START_ADDRESS_WIDTH
    {
        Zero = 0,
        One,
        Two,
        Three,
        Four, /* Five ~ Sixtythree */
        Max = 64
    };

    /**
     * @brief
     * It is used to store the migrated page's migration granularity.
     */
    enum class MigrationGranularity : MIGRATION_GRANULARITY_WIDTH
    {
        None     = 0,
        Byte_64  = 1,
        Byte_128 = 2,
        Byte_256 = 4,
        Byte_512 = 8,
        KiB_1    = 16,
        KiB_2    = 32,
        KiB_4    = 64,
        Max
    };

    struct AccessDistribution
    {
        bool access[START_ADDRESS_WIDTH(StartAddress::Max)];

#if (STATISTICS_INFORMATION == ENABLE)
        bool access_flag = false;                                           // Mark whether this page has ever been accessed.
        bool access_stats[START_ADDRESS_WIDTH(StartAddress::Max)];          // Record the accessed data line
        bool temporal_access_stats[START_ADDRESS_WIDTH(StartAddress::Max)]; // Record the accessed data line for a fix intervsal (clear is possible)
        MIGRATION_GRANULARITY_WIDTH estimated_spatial_locality_stats = 0;   // Store the estimated spatial locality for this page
        MIGRATION_GRANULARITY_WIDTH granularity_stats                = 0;   // Store the best granularity for this page
        MIGRATION_GRANULARITY_WIDTH granularity_predict_stats        = 0;   // Store the predicted granularity for this page

#endif /* STATISTICS_INFORMATION */

        AccessDistribution()
        {
            for (uint8_t i = 0; i < START_ADDRESS_WIDTH(StartAddress::Max); i++)
            {
                access[i] = false;
            }

#if (STATISTICS_INFORMATION == ENABLE)
            for (uint8_t i = 0; i < START_ADDRESS_WIDTH(StartAddress::Max); i++)
            {
                access_stats[i]          = false;
                temporal_access_stats[i] = false;
            }
#endif /* STATISTICS_INFORMATION */
        };
    };

    /* Access distribution table */
    std::vector<AccessDistribution>& access_table; // A access distribution for every data block

    struct PlacementEntry
    {
        REMAPPING_LOCATION_WIDTH cursor; // This cursor is used to track the position of next available group for migration in the placement entry.
        REMAPPING_LOCATION_WIDTH tag[NUMBER_OF_BLOCK];
        START_ADDRESS_WIDTH start_address[NUMBER_OF_BLOCK];
        MIGRATION_GRANULARITY_WIDTH granularity[NUMBER_OF_BLOCK];

        PlacementEntry()
        {
            cursor = 0;

            for (uint8_t i = 0; i < NUMBER_OF_BLOCK; i++)
            {
                tag[i]           = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);
                start_address[i] = START_ADDRESS_WIDTH(StartAddress::Zero);
                granularity[i]   = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
            }
        };
    };

    /* Remapping table */
    std::vector<PlacementEntry>& placement_table;

    uint64_t expected_number_in_congruence_group = 0;

    /* Member functions */
    OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address);
    ~OS_TRANSPARENT_MANAGEMENT();

    // Address is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, MemoryRequestType type, access_type type_origin, float queue_busy_degree);

    // Translate the physical address to hardware address
    void physical_to_hardware_address(request_type& packet);
    void physical_to_hardware_address(uint64_t& address);

    bool finish_remapping_request() override;

    // Detect cold data block
    void cold_data_detection();

    /**
     * @brief Operate during a fixed time interval (Epoch) of swapping.
     */
    void check_interval_swap([[maybe_unused]] uint8_t swapping_states, [[maybe_unused]] bool warmup) {};

    // Member functions for migration granularity:

    // Calculate the migration granularity based on start_address and end_address.
    static MIGRATION_GRANULARITY_WIDTH calculate_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address);
    // Check whether this migration granularity is beyond the block's range and adjust it to a proper value, this function returns updated end_address
    static START_ADDRESS_WIDTH adjust_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity);
    // Check whether this migration granularity is beyond the block's end address and adjust it to a proper value, this function returns updated end_address
    static START_ADDRESS_WIDTH round_down_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity);

private:
#if (COLD_DATA_DETECTION_IN_GROUP == ENABLE)
    // Detect cold data block in group
    void cold_data_detection_in_group(uint64_t source_address);
#endif /* COLD_DATA_DETECTION_IN_GROUP */

    // Evict cold data block
    bool cold_data_eviction(uint64_t source_address, float queue_busy_degree);

    // Add new remapping request into the remapping_request_queue
    bool enqueue_remapping_request(RemappingRequest& remapping_request);

    /**
     * @brief Shrink migration_granularity until the migrated part fits below limit
     *
     * @param limit                The last data line position the migration may cover.
     * @param original_end_address The end address the caller was asked for, used as-is by
     *                             FLEXIBLE_GRANULARITY to fall back to an exact-size migration.
     * @return The updated end address.
     */
    static START_ADDRESS_WIDTH shrink_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH limit, MIGRATION_GRANULARITY_WIDTH& migration_granularity, const START_ADDRESS_WIDTH original_end_address);
};

#endif /* IDEAL_VARIABLE_GRANULARITY */

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
#endif /* VARIABLE_GRANULARITY_H */

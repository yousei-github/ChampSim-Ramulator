#ifndef OS_TRANSPARENT_MANAGEMENT_COMMON_H
#define OS_TRANSPARENT_MANAGEMENT_COMMON_H

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <vector>

#include "ChampSim/access_type.h"
#include "ChampSim/champsim_constants.h"
#include "ChampSim/channel.h"
#include "ChampSim/util/bits.h"
#include "ProjectConfiguration.h" // User file

/**
 * @note Abbreviation:
 * FM -> Fast memory (e.g., HBM, DDR4)
 * SM -> Slow memory (e.g., DDR4, PCM)
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

/**
 * @brief
 * Definitions shared by every research proposal (OS-transparent management design).
 * A proposal-specific header (cameo.h, variable_granularity.h, ideal_single_mempod.h, ...) is responsible only for what actually differs between proposals; 
 * anything that was identical in all of them lives here.
 */

#define COUNTER_WIDTH                         uint8_t
#define COUNTER_MAX_VALUE                     (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE                 (0)

#define HOTNESS_WIDTH                         bool
#define HOTNESS_DEFAULT_VALUE                 (false)

#define REMAPPING_LOCATION_WIDTH              uint8_t

#define QUEUE_BUSY_DEGREE_THRESHOLD           (0.8f)

#define INCOMPLETE_READ_REQUEST_QUEUE_LENGTH  (128)
#define INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH (128)

/**
 * @brief
 * The state and behaviour every OS_TRANSPARENT_MANAGEMENT shares.
 * Every research proposal inherit from this publicly
 *
 * @note
 * Exactly one proposal is compiled in, so the class is never used polymorphically.
 */
class OS_TRANSPARENT_MANAGEMENT_BASE
{
public:
    /** @brief Memory request type */
    enum class MemoryRequestType : int
    {
        Read = 0,
        Write,
        Max
    };

    /**
     * @brief Remapping request
     *
     * @note
     * The fields are the combination of what the proposals need.
     *
     * Every proposal fills in the hardware addresses and the size;
     * the remaining fields are proposal-specific and keep their default value where they carry no meaning:
     * - p_address_in_fm / p_address_in_sm: physical addresses, IDEAL_SINGLE_MEMPOD only.
     * - fm_location / sm_location: positions inside the remapping table,
     *   that is the line location table entry in a congruence group (CAMEO) or the placement table entry (variable granularity).
     */
    struct RemappingRequest
    {
        uint64_t h_address_in_fm = 0, h_address_in_sm = 0;         // Hardware address in fast and slow memories
        uint64_t p_address_in_fm = 0, p_address_in_sm = 0;         // Physical address in fast and slow memories
        REMAPPING_LOCATION_WIDTH fm_location = 0, sm_location = 0; // Positions in the remapping table
        uint8_t size = 0;                                          // Number of cache lines to remap
    };

    uint64_t cycle = 0;
    uint64_t total_capacity;       // Unit is byte
    uint64_t fast_memory_capacity; // Unit is byte
    uint64_t total_capacity_at_data_block_granularity;
    uint64_t fast_memory_capacity_at_data_block_granularity;
    // Address format in the data management granularity
    uint8_t fast_memory_offset_bit;

    std::deque<RemappingRequest> remapping_request_queue;
    uint64_t remapping_request_queue_congestion = 0;

    /**
     * @brief Get a remapping request at the head of the remapping_request_queue
     *
     * @note
     * The request is only peeked at, not dequeued.
     * The memory controller keeps processing the same request until it tells the proposal the swapping is over via finish_remapping_request().
     */
    virtual bool issue_remapping_request(RemappingRequest& remapping_request)
    {
        if (remapping_request_queue.empty() == false)
        {
            remapping_request = remapping_request_queue.front();
            return true;
        }

        return false;
    };

    virtual bool finish_remapping_request() = 0;

protected:
    /**
     * @param[in] data_management_offset_bits The width of offset bits of data block management granularity
     * @param[in] fm_offset_bit The width of offset bits of fast memory
     */
    OS_TRANSPARENT_MANAGEMENT_BASE(uint64_t max_address, uint64_t fast_memory_max_address, uint8_t data_management_offset_bits, uint8_t fm_offset_bit)
    : total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
      total_capacity_at_data_block_granularity(max_address >> data_management_offset_bits),
      fast_memory_capacity_at_data_block_granularity(fast_memory_max_address >> data_management_offset_bits),
      fast_memory_offset_bit(fm_offset_bit)
    {
    }

    ~OS_TRANSPARENT_MANAGEMENT_BASE() = default;
};

namespace OsTransparentManagement
{
/**
 * @brief Whether this access is excluded from activity tracking
 *
 * @note
 * Always false unless TRACKING_LOAD_STORE_STATISTICS is enabled, in which case
 * TRACKING_LOAD_ONLY / TRACKING_READ_ONLY narrow down what the proposal is allowed to see.
 */
inline bool should_skip_tracking([[maybe_unused]] OS_TRANSPARENT_MANAGEMENT_BASE::MemoryRequestType type, [[maybe_unused]] access_type type_origin)
{
#if (TRACKING_LOAD_ONLY)
    if (type_origin == access_type::RFO || type_origin == access_type::WRITE) // CPU Store Instruction and LLC Writeback is ignored
    {
        return true;
    }
#endif /* TRACKING_LOAD_ONLY */

#if (TRACKING_READ_ONLY)
    if (type == OS_TRANSPARENT_MANAGEMENT_BASE::MemoryRequestType::Write) // Memory Write is ignored
    {
        return true;
    }
#endif /* TRACKING_READ_ONLY */

    return false;
}

/** @brief Count an access to a data block and mark the block hot once it reaches the threshold */
inline void update_counter_and_hotness(std::vector<COUNTER_WIDTH>& counter_table, std::vector<HOTNESS_WIDTH>& hotness_table, uint64_t data_block_address, COUNTER_WIDTH hotness_threshold)
{
    if (counter_table.at(data_block_address) < COUNTER_MAX_VALUE)
    {
        counter_table[data_block_address]++; // Increment its counter
    }

    if (counter_table.at(data_block_address) >= hotness_threshold)
    {
        hotness_table.at(data_block_address) = true; // Mark hot data block
    }
}
} // namespace OsTransparentManagement

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
#endif /* OS_TRANSPARENT_MANAGEMENT_COMMON_H */

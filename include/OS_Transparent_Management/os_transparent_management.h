#ifndef OS_TRANSPARENT_MANAGEMENT_H
#define OS_TRANSPARENT_MANAGEMENT_H
#include <cassert>
#include <concepts>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <vector>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/util/bits.h"
#include "OS_Transparent_Management/os_transparent_management_common.h"
#include "ProjectConfiguration.h" // User file

/* Includes for research */
#include "OS_Transparent_Management/cameo.h"
#include "OS_Transparent_Management/ideal_single_mempod.h"
#include "OS_Transparent_Management/variable_granularity.h"

/**
 * @note Abbreviation:
 * FM -> Fast memory (e.g., HBM, DDR4)
 * SM -> Slow memory (e.g., DDR4, PCM)
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (NO_METHOD_FOR_RUN_HYBRID_MEMORY == ENABLE)

/**
 * @brief
 * The baseline for the hybrid memory system: data is placed statically and never migrated,
 * so a run measures what the research proposals in this directory have to beat.
 *
 * Selected by disabling every proposal macro in the "Research proposal selection" block of
 * ProjectConfiguration.h. The physical address is the hardware address, hence the identity
 * translation below, and no remapping request is ever produced.
 */
class OS_TRANSPARENT_MANAGEMENT : public OS_TRANSPARENT_MANAGEMENT_BASE
{
    using channel_type = champsim::channel;
    using request_type = typename channel_type::request_type;

public:
    COUNTER_WIDTH hotness_threshold = 0;

    std::vector<COUNTER_WIDTH>& counter_table; // A counter for every data block
    std::vector<HOTNESS_WIDTH>& hotness_table; // A hotness bit for every data block, true -> data block is hot, false -> data block is cold.

    /* Member functions */
    OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address);
    ~OS_TRANSPARENT_MANAGEMENT();

    // Adress is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, MemoryRequestType type, access_type type_origin, float queue_busy_degree);

    // Translate the physical address to hardware address
    void physical_to_hardware_address(request_type& packet);
    void physical_to_hardware_address(uint64_t& address);

    bool finish_remapping_request();

    // Detect cold data block
    void cold_data_detection();

    /**
     * @brief Epoch hook, only IDEAL_SINGLE_MEMPOD migrates on a fixed time interval.
     * @note Part of the interface every proposal exposes, so the memory controller can
     *       call it without knowing which proposal is compiled in.
     */
    void check_interval_swap([[maybe_unused]] uint8_t swapping_states, [[maybe_unused]] bool warmup) {};

private:
    // Evict cold data block
    bool cold_data_eviction(uint64_t source_address, float queue_busy_degree);

    // Add new remapping request into the remapping_request_queue
    bool enqueue_remapping_request(RemappingRequest& remapping_request);
};

#endif /* NO_METHOD_FOR_RUN_HYBRID_MEMORY */

/**
 * @brief The interface the memory controller drives an OS-transparent management design through
 *
 * @note
 * Every research proposal implements the same set of member functions, so the memory
 * controller needs no #if of its own to call them. The static_assert below checks the
 * proposal that is actually compiled in, which turns "I added a proposal and forgot a
 * hook" from a wall of template errors at the call site into one readable message here.
 */
template<typename OTM>
concept os_transparent_management_policy =
    std::derived_from<OTM, OS_TRANSPARENT_MANAGEMENT_BASE> && requires(OTM otm, typename OTM::RemappingRequest remapping_request, champsim::channel::request_type packet, uint64_t address, float queue_busy_degree, uint8_t swapping_states, bool warmup, access_type type_origin) {
        { otm.memory_activity_tracking(address, OTM::MemoryRequestType::Read, type_origin, queue_busy_degree) } -> std::same_as<bool>;
        { otm.physical_to_hardware_address(packet) } -> std::same_as<void>;
        { otm.physical_to_hardware_address(address) } -> std::same_as<void>;
        { otm.issue_remapping_request(remapping_request) } -> std::same_as<bool>;
        { otm.finish_remapping_request() } -> std::same_as<bool>;
        { otm.cold_data_detection() } -> std::same_as<void>;
        { otm.check_interval_swap(swapping_states, warmup) } -> std::same_as<void>;
    };

static_assert(os_transparent_management_policy<OS_TRANSPARENT_MANAGEMENT>,
    "The selected OS-transparent management design does not implement the interface the memory controller expects.");

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
#endif /* OS_TRANSPARENT_MANAGEMENT_H */

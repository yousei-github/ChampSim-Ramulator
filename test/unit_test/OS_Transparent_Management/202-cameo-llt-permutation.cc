#include <algorithm>
#include <catch.hpp>
#include <vector>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && ((IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE))
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

namespace
{
constexpr float idle_queue = 0.0f;

/** @brief Where each member of a congruence group currently lives */
std::vector<uint64_t> translate_congruence_group(OS_TRANSPARENT_MANAGEMENT& os_transparent_management, uint64_t group)
{
    std::vector<uint64_t> hardware_addresses;
    for (uint64_t location = 0; location < otm_test::congruence_group_size; location++)
    {
        uint64_t address = otm_test::congruence_group_member_address(group, location);
        os_transparent_management.physical_to_hardware_address(address);
        hardware_addresses.push_back(address);
    }

    return hardware_addresses;
}
} // namespace

/**
 * A line location table entry is a permutation of the congruence group's members: a migration
 * swaps two of its slots, so no two members may ever share a hardware address and no member may
 * leave the group. That is the invariant finish_remapping_request() checks by checksum under
 * NDEBUG; here it is checked through the public translation instead.
 */
SCENARIO("A finished CAMEO migration exchanges the two data blocks it swapped")
{
    GIVEN("A CAMEO design with a hot slow-memory data block queued for migration")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        const uint64_t group                        = 11;
        const uint64_t location                     = 3;
        const uint64_t slow_memory_address          = otm_test::congruence_group_member_address(group, location);
        const uint64_t fast_memory_address          = otm_test::congruence_group_member_address(group, 0);

        const std::vector<uint64_t> before_swapping = translate_congruence_group(os_transparent_management, group);

        for (COUNTER_WIDTH i = 0; i < HOTNESS_THRESHOLD; i++)
        {
            os_transparent_management.memory_activity_tracking(slow_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
        }
        REQUIRE(os_transparent_management.remapping_request_queue.size() == 1);

        WHEN("The memory controller reports the migration as finished")
        {
            REQUIRE(os_transparent_management.finish_remapping_request() == true);

            THEN("The migration leaves the queue")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
            }

            THEN("The two data blocks have exchanged places")
            {
                uint64_t migrated_block = slow_memory_address;
                os_transparent_management.physical_to_hardware_address(migrated_block);
                REQUIRE(migrated_block == fast_memory_address);

                uint64_t evicted_block = fast_memory_address;
                os_transparent_management.physical_to_hardware_address(evicted_block);
                REQUIRE(evicted_block == slow_memory_address);
            }

            THEN("The congruence group is still a permutation of the same hardware addresses")
            {
                std::vector<uint64_t> after_swapping = translate_congruence_group(os_transparent_management, group);
                REQUIRE(after_swapping != before_swapping);

                std::vector<uint64_t> sorted_before = before_swapping, sorted_after = after_swapping;
                std::sort(sorted_before.begin(), sorted_before.end());
                std::sort(sorted_after.begin(), sorted_after.end());
                REQUIRE(sorted_after == sorted_before);
            }

            THEN("No other congruence group moved")
            {
                const uint64_t untouched_group = group + 1;
                for (uint64_t untouched_location = 0; untouched_location < otm_test::congruence_group_size; untouched_location++)
                {
                    const uint64_t physical_address = otm_test::congruence_group_member_address(untouched_group, untouched_location);

                    uint64_t hardware_address       = physical_address;
                    os_transparent_management.physical_to_hardware_address(hardware_address);
                    REQUIRE(hardware_address == physical_address);
                }
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE */

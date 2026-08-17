#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (NO_METHOD_FOR_RUN_HYBRID_MEMORY == ENABLE)
#include "ChampSim/channel.h"
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

namespace
{
constexpr float idle_queue = 0.0f;
} // namespace

/**
 * The baseline never migrates anything, so the hardware address is always the physical address.
 * The packet overload is what the memory controller dispatches on: leaving packet.h_address at
 * its default would send every request to the start of fast memory.
 */
SCENARIO("The static placement baseline hands the physical address straight to the memory")
{
    GIVEN("A freshly constructed static placement baseline")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        WHEN("Addresses across every memory tier are translated")
        {
            THEN("Each hardware address equals the physical address it came from")
            {
                for (uint64_t location = 0; location < otm_test::congruence_group_size; location++)
                {
                    for (uint64_t group : {uint64_t {0}, uint64_t {1}, uint64_t {37}, otm_test::fast_memory_data_blocks - 1})
                    {
                        const uint64_t physical_address = otm_test::congruence_group_member_address(group, location);

                        champsim::channel::request_type packet;
                        packet.address = champsim::address {physical_address};
                        os_transparent_management.physical_to_hardware_address(packet);
                        REQUIRE(packet.h_address == physical_address);

                        uint64_t hardware_address = physical_address;
                        os_transparent_management.physical_to_hardware_address(hardware_address);
                        REQUIRE(hardware_address == physical_address);
                    }
                }
            }
        }
    }
}

SCENARIO("The static placement baseline never asks for a migration")
{
    GIVEN("A freshly constructed static placement baseline")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        WHEN("A slow memory address is accessed repeatedly")
        {
            const uint64_t slow_memory_address = otm_test::congruence_group_member_address(7, 2);
            for (int i = 0; i < 16; i++)
            {
                REQUIRE(os_transparent_management.memory_activity_tracking(slow_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue) == true);
            }

            THEN("No migration is queued and none is dropped")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
                REQUIRE(os_transparent_management.remapping_request_queue_congestion == 0);

                OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
                REQUIRE(os_transparent_management.issue_remapping_request(remapping_request) == false);
            }
        }

        WHEN("An address past the end of the hybrid memory is tracked")
        {
            THEN("The access is reported as invalid")
            {
                REQUIRE(os_transparent_management.memory_activity_tracking(otm_test::total_capacity, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue) == false);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, NO_METHOD_FOR_RUN_HYBRID_MEMORY */

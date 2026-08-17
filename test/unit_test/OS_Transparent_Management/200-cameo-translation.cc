#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && ((IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE))
#include "ChampSim/channel.h"
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

/**
 * A fresh line location table maps every member of a congruence group to its own memory tier
 * (LocationTableEntry's constructor fills location[i] with i), so before anything migrates the
 * hardware address must equal the physical address.
 */
SCENARIO("CAMEO translates a physical address to itself before any data migrates")
{
    GIVEN("A freshly constructed CAMEO design")
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

                        uint64_t hardware_address       = physical_address;
                        os_transparent_management.physical_to_hardware_address(hardware_address);
                        REQUIRE(hardware_address == physical_address);
                    }
                }
            }
        }

        WHEN("The same address is translated through both overloads")
        {
            const uint64_t physical_address = otm_test::congruence_group_member_address(3, 2);

            uint64_t hardware_address       = physical_address;
            os_transparent_management.physical_to_hardware_address(hardware_address);

            champsim::channel::request_type packet;
            packet.address = champsim::address {physical_address};
            os_transparent_management.physical_to_hardware_address(packet);

            THEN("Both overloads agree")
            {
                REQUIRE(packet.h_address == hardware_address);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE */

#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

/**
 * With the line location table co-located in fast memory, a request first fetches the Location
 * Entry and Data (LEAD) from fast memory and only then reaches its real destination. It waits in
 * an incomplete request queue in between; the memory controller marks the fast memory half done
 * when that access returns.
 */
SCENARIO("A co-located CAMEO design marks the fast memory half of a read as finished exactly once")
{
    GIVEN("A design with two reads waiting on the same hardware address")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        const uint64_t hardware_address = otm_test::congruence_group_member_address(5, 0);
        for (int i = 0; i < 2; i++)
        {
            OS_TRANSPARENT_MANAGEMENT::ReadRequest read_request;
            read_request.packet.h_address = hardware_address;
            os_transparent_management.incomplete_read_request_queue.push_back(read_request);
        }

        WHEN("The fast memory access returns twice")
        {
            const bool first  = os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(hardware_address);
            const bool second = os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(hardware_address);

            THEN("Each waiting read is marked done once")
            {
                REQUIRE(first == true);
                REQUIRE(second == true);
                REQUIRE(os_transparent_management.incomplete_read_request_queue[0].fm_access_finish == true);
                REQUIRE(os_transparent_management.incomplete_read_request_queue[1].fm_access_finish == true);
            }

            AND_WHEN("It returns once more")
            {
                THEN("Nothing is left to mark")
                {
                    REQUIRE(os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(hardware_address) == false);
                }
            }
        }

        WHEN("A fast memory access for an address nothing is waiting on returns")
        {
            THEN("No waiting read is marked done")
            {
                REQUIRE(os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(hardware_address + DATA_MANAGEMENT_GRANULARITY) == false);
                REQUIRE(os_transparent_management.incomplete_read_request_queue[0].fm_access_finish == false);
                REQUIRE(os_transparent_management.incomplete_read_request_queue[1].fm_access_finish == false);
            }
        }
    }
}

SCENARIO("A co-located CAMEO design marks the fast memory half of a write as finished exactly once")
{
    GIVEN("A design with one write waiting on a hardware address")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        const uint64_t hardware_address = otm_test::congruence_group_member_address(9, 0);
        OS_TRANSPARENT_MANAGEMENT::WriteRequest write_request;
        write_request.packet.h_address = hardware_address;
        os_transparent_management.incomplete_write_request_queue.push_back(write_request);

        WHEN("The fast memory access returns")
        {
            const bool finished = os_transparent_management.finish_fm_access_in_incomplete_write_request_queue(hardware_address);

            THEN("The waiting write is marked done, and only once")
            {
                REQUIRE(finished == true);
                REQUIRE(os_transparent_management.incomplete_write_request_queue[0].fm_access_finish == true);
                REQUIRE(os_transparent_management.finish_fm_access_in_incomplete_write_request_queue(hardware_address) == false);
            }
        }

        WHEN("A fast memory access for an unrelated address returns")
        {
            THEN("The waiting write is left alone")
            {
                REQUIRE(os_transparent_management.finish_fm_access_in_incomplete_write_request_queue(hardware_address + DATA_MANAGEMENT_GRANULARITY) == false);
                REQUIRE(os_transparent_management.incomplete_write_request_queue[0].fm_access_finish == false);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, COLOCATED_LINE_LOCATION_TABLE */

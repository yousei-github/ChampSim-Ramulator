#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

/**
 * issue_remapping_request() is shared by every research proposal (it lives in
 * OS_TRANSPARENT_MANAGEMENT_BASE), so these run whichever proposal is compiled in.
 */

SCENARIO("An OS-transparent management design issues no remapping request while its queue is empty")
{
    GIVEN("A freshly constructed OS-transparent management design")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        THEN("The remapping request queue is empty")
        {
            REQUIRE(os_transparent_management.remapping_request_queue.empty());
            REQUIRE(os_transparent_management.remapping_request_queue_congestion == 0);
        }

        WHEN("A remapping request is issued")
        {
            OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
            const bool issued = os_transparent_management.issue_remapping_request(remapping_request);

            THEN("No request is handed out")
            {
                REQUIRE(issued == false);
            }
        }
    }
}

SCENARIO("Issuing a remapping request peeks at the queue instead of consuming it")
{
    GIVEN("An OS-transparent management design with one queued remapping request")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        OS_TRANSPARENT_MANAGEMENT::RemappingRequest queued_request;
        queued_request.h_address_in_fm = 0;
        queued_request.h_address_in_sm = otm_test::fast_memory_capacity;
        queued_request.size            = 1;
        os_transparent_management.remapping_request_queue.push_back(queued_request);

        WHEN("The same request is issued twice")
        {
            OS_TRANSPARENT_MANAGEMENT::RemappingRequest first_request, second_request;
            const bool first_issued  = os_transparent_management.issue_remapping_request(first_request);
            const bool second_issued = os_transparent_management.issue_remapping_request(second_request);

            THEN("Both issues hand out the head of the queue, which stays queued")
            {
                REQUIRE(first_issued == true);
                REQUIRE(second_issued == true);
                REQUIRE(first_request.h_address_in_fm == queued_request.h_address_in_fm);
                REQUIRE(first_request.h_address_in_sm == queued_request.h_address_in_sm);
                REQUIRE(second_request.h_address_in_fm == queued_request.h_address_in_fm);
                REQUIRE(second_request.h_address_in_sm == queued_request.h_address_in_sm);
                REQUIRE(os_transparent_management.remapping_request_queue.size() == 1);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

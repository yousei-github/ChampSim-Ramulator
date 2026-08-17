#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && ((IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE))
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

namespace
{
// Below QUEUE_BUSY_DEGREE_THRESHOLD, so a hot data block is always allowed to enqueue.
constexpr float idle_queue = 0.0f;
} // namespace

SCENARIO("CAMEO asks to migrate a data block once it becomes hot in slow memory")
{
    GIVEN("A freshly constructed CAMEO design")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        WHEN("A data block that lives in fast memory is accessed enough to be hot")
        {
            const uint64_t fast_memory_address = otm_test::congruence_group_member_address(7, 0);
            for (COUNTER_WIDTH i = 0; i < HOTNESS_THRESHOLD; i++)
            {
                os_transparent_management.memory_activity_tracking(fast_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
            }

            THEN("No migration is requested, the data is already where it should be")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
            }
        }

        WHEN("A data block that lives in slow memory is accessed enough to be hot")
        {
            const uint64_t group               = 7;
            const uint64_t location            = 2;
            const uint64_t slow_memory_address = otm_test::congruence_group_member_address(group, location);
            for (COUNTER_WIDTH i = 0; i < HOTNESS_THRESHOLD; i++)
            {
                os_transparent_management.memory_activity_tracking(slow_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
            }

            THEN("It is queued to swap with the group member currently in fast memory")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.size() == 1);

                OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
                REQUIRE(os_transparent_management.issue_remapping_request(remapping_request) == true);
                REQUIRE(remapping_request.h_address_in_fm == otm_test::congruence_group_member_address(group, 0));
                REQUIRE(remapping_request.h_address_in_sm == slow_memory_address);
                REQUIRE(remapping_request.fm_location == 0);
                REQUIRE(remapping_request.sm_location == location);
                REQUIRE(remapping_request.size == DATA_GRANULARITY_IN_CACHE_LINE);
            }

            AND_WHEN("The same data block is accessed again")
            {
                os_transparent_management.memory_activity_tracking(slow_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Write, access_type::WRITE, idle_queue);

                THEN("The congruence group is not queued a second time")
                {
                    REQUIRE(os_transparent_management.remapping_request_queue.size() == 1);
                }
            }
        }

        WHEN("A hot data block in slow memory is accessed while the memory queues are busy")
        {
            const uint64_t slow_memory_address = otm_test::congruence_group_member_address(7, 2);
            for (COUNTER_WIDTH i = 0; i < HOTNESS_THRESHOLD; i++)
            {
                os_transparent_management.memory_activity_tracking(slow_memory_address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, QUEUE_BUSY_DEGREE_THRESHOLD + 0.1f);
            }

            THEN("No migration is requested, the memory is too busy to spend bandwidth on it")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
            }
        }
    }
}

SCENARIO("CAMEO counts the migrations it has to drop when its request queue is full")
{
    GIVEN("A CAMEO design whose remapping request queue is exactly full")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        // Each congruence group is enqueued at most once, so distinct groups fill the queue.
        for (uint64_t group = 0; group < REMAPPING_REQUEST_QUEUE_LENGTH; group++)
        {
            os_transparent_management.memory_activity_tracking(otm_test::congruence_group_member_address(group, 1), OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
        }

        REQUIRE(os_transparent_management.remapping_request_queue.size() == REMAPPING_REQUEST_QUEUE_LENGTH);
        REQUIRE(os_transparent_management.remapping_request_queue_congestion == 0);

        WHEN("One more congruence group turns hot in slow memory")
        {
            os_transparent_management.memory_activity_tracking(otm_test::congruence_group_member_address(REMAPPING_REQUEST_QUEUE_LENGTH, 1), OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);

            THEN("The queue does not grow and the dropped migration is counted")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.size() == REMAPPING_REQUEST_QUEUE_LENGTH);
                REQUIRE(os_transparent_management.remapping_request_queue_congestion == 1);
            }
        }
    }
}

SCENARIO("CAMEO rejects an access beyond the memory it manages")
{
    GIVEN("A freshly constructed CAMEO design")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        WHEN("An address past the end of the hybrid memory is tracked")
        {
            const bool tracked = os_transparent_management.memory_activity_tracking(otm_test::total_capacity, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);

            THEN("The access is reported as invalid")
            {
                REQUIRE(tracked == false);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE */

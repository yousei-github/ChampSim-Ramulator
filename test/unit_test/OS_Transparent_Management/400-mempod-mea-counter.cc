#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (IDEAL_SINGLE_MEMPOD == ENABLE)
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

namespace
{
constexpr float idle_queue = 0.0f;

void track_read(OS_TRANSPARENT_MANAGEMENT& os_transparent_management, uint64_t address)
{
    os_transparent_management.memory_activity_tracking(address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
}
} // namespace

/**
 * MemPod tracks the hottest data segments with a Misra-Gries ("majority element") counter table:
 * a hit increments, and a miss on a full table decrements every counter instead of evicting one.
 */
SCENARIO("MemPod counts accesses to a data segment up to its counter's maximum")
{
    GIVEN("A freshly constructed MemPod design")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        THEN("Nothing is being tracked yet")
        {
            REQUIRE(os_transparent_management.mea_counter_table.empty());
        }

        WHEN("A data segment is accessed for the first time")
        {
            const uint64_t segment = 3;
            track_read(os_transparent_management, otm_test::data_block_address(segment));

            THEN("It starts being tracked")
            {
                REQUIRE(os_transparent_management.mea_counter_table.size() == 1);
                REQUIRE(os_transparent_management.mea_counter_table.at(segment) == 1);
            }

            AND_WHEN("It is accessed far more often than its counter can hold")
            {
                for (int i = 0; i < 4 * MEA_COUNTER_MAX_VALUE; i++)
                {
                    track_read(os_transparent_management, otm_test::data_block_address(segment));
                }

                THEN("Its counter saturates instead of wrapping")
                {
                    REQUIRE(os_transparent_management.mea_counter_table.at(segment) == MEA_COUNTER_MAX_VALUE + 1u);
                }
            }
        }

        WHEN("Every access falls inside one data segment")
        {
            const uint64_t segment = 3;
            track_read(os_transparent_management, otm_test::data_block_address(segment));
            track_read(os_transparent_management, otm_test::data_block_address(segment) + DATA_MANAGEMENT_GRANULARITY - 1);

            THEN("They are counted against the same segment")
            {
                REQUIRE(os_transparent_management.mea_counter_table.size() == 1);
                REQUIRE(os_transparent_management.mea_counter_table.at(segment) == 2);
            }
        }
    }
}

SCENARIO("MemPod decays every counter when a new data segment finds the table full")
{
    GIVEN("A MemPod design tracking as many data segments as it has counters")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        for (uint64_t segment = 0; segment < NUMBER_MEA_COUNTER; segment++)
        {
            track_read(os_transparent_management, otm_test::data_block_address(segment));
        }

        REQUIRE(os_transparent_management.mea_counter_table.size() == NUMBER_MEA_COUNTER);

        WHEN("A data segment that is not tracked yet is accessed")
        {
            track_read(os_transparent_management, otm_test::data_block_address(NUMBER_MEA_COUNTER));

            THEN("Every counter is decayed, and the ones that hit zero stop being tracked")
            {
                REQUIRE(os_transparent_management.mea_counter_table.empty());
            }
        }

        WHEN("One tracked data segment is much hotter than the rest")
        {
            const uint64_t hot_segment = 0;
            for (int i = 0; i < MEA_COUNTER_MAX_VALUE; i++)
            {
                track_read(os_transparent_management, otm_test::data_block_address(hot_segment));
            }

            AND_WHEN("A data segment that is not tracked yet is accessed")
            {
                track_read(os_transparent_management, otm_test::data_block_address(NUMBER_MEA_COUNTER));

                THEN("Only the hot data segment survives the decay")
                {
                    REQUIRE(os_transparent_management.mea_counter_table.size() == 1);
                    REQUIRE(os_transparent_management.mea_counter_table.count(hot_segment) == 1);
                    REQUIRE(os_transparent_management.mea_counter_table.at(hot_segment) == MEA_COUNTER_MAX_VALUE);
                }
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_SINGLE_MEMPOD */

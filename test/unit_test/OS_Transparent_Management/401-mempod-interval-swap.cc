#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (IDEAL_SINGLE_MEMPOD == ENABLE)
#include "OS_Transparent_Management/os_transparent_management.h"
#include "test_capacities.hpp"

namespace
{
constexpr float idle_queue   = 0.0f;
constexpr uint8_t swap_idle  = 0; // The swapping unit is idle
constexpr bool not_in_warmup = false;

void track_read(OS_TRANSPARENT_MANAGEMENT& os_transparent_management, uint64_t address)
{
    os_transparent_management.memory_activity_tracking(address, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read, access_type::LOAD, idle_queue);
}

uint64_t translate(OS_TRANSPARENT_MANAGEMENT& os_transparent_management, uint64_t address)
{
    os_transparent_management.physical_to_hardware_address(address);
    return address;
}
} // namespace

SCENARIO("MemPod translates a physical address to itself before any data migrates")
{
    GIVEN("A freshly constructed MemPod design")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        THEN("Every address maps to itself")
        {
            for (uint64_t segment : {uint64_t {0}, uint64_t {1}, otm_test::fast_memory_data_blocks, otm_test::fast_memory_data_blocks * otm_test::congruence_group_size - 1})
            {
                const uint64_t physical_address = otm_test::data_block_address(segment);
                REQUIRE(translate(os_transparent_management, physical_address) == physical_address);
            }
        }

        THEN("An offset inside a data segment is preserved")
        {
            const uint64_t physical_address = otm_test::data_block_address(2) + 64;
            REQUIRE(translate(os_transparent_management, physical_address) == physical_address);
        }
    }
}

/**
 * MemPod does not migrate on demand: it collects hot data segments and, once per fixed time
 * interval, swaps the hot ones that sit in slow memory with fast memory segments picked round
 * robin.
 */
SCENARIO("MemPod migrates hot slow-memory data only when its interval elapses")
{
    GIVEN("A MemPod design with one hot data segment in slow memory")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        const uint64_t slow_memory_segment = otm_test::fast_memory_data_blocks + 5;
        const uint64_t slow_memory_address = otm_test::data_block_address(slow_memory_segment);
        track_read(os_transparent_management, slow_memory_address);

        WHEN("The interval has not elapsed yet")
        {
            os_transparent_management.check_interval_swap(swap_idle, not_in_warmup);

            THEN("Nothing is migrated")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
            }
        }

        WHEN("The interval elapses")
        {
            os_transparent_management.cycle = static_cast<uint64_t>(os_transparent_management.next_interval_cycle) + 1;
            os_transparent_management.check_interval_swap(swap_idle, not_in_warmup);

            THEN("The hot data segment is queued to swap with a fast memory segment")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.size() == 1);

                OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
                REQUIRE(os_transparent_management.issue_remapping_request(remapping_request) == true);
                REQUIRE(remapping_request.h_address_in_sm == slow_memory_address);
                REQUIRE(remapping_request.p_address_in_sm == slow_memory_address);
                REQUIRE(remapping_request.h_address_in_fm < otm_test::fast_memory_capacity);
                REQUIRE(remapping_request.size == SWAP_DATA_CACHE_LINES);
            }

            THEN("The next interval is scheduled")
            {
                REQUIRE(os_transparent_management.next_interval_cycle > os_transparent_management.cycle);
            }

            AND_WHEN("The memory controller reports the swap as finished")
            {
                OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
                os_transparent_management.issue_remapping_request(remapping_request);
                const uint64_t victim_address = remapping_request.p_address_in_fm;

                REQUIRE(os_transparent_management.finish_remapping_request() == true);

                THEN("The hot data is now reached through fast memory, and the victim through slow memory")
                {
                    REQUIRE(translate(os_transparent_management, slow_memory_address) == remapping_request.h_address_in_fm);
                    REQUIRE(translate(os_transparent_management, victim_address) == slow_memory_address);
                    REQUIRE(os_transparent_management.remapping_request_queue.empty());
                }
            }
        }
    }
}

SCENARIO("MemPod leaves hot fast-memory data where it is")
{
    GIVEN("A MemPod design whose only hot data segment already lives in fast memory")
    {
        OS_TRANSPARENT_MANAGEMENT os_transparent_management {otm_test::total_capacity, otm_test::fast_memory_capacity};

        const uint64_t fast_memory_address = otm_test::data_block_address(5);
        track_read(os_transparent_management, fast_memory_address);

        WHEN("The interval elapses")
        {
            os_transparent_management.cycle = static_cast<uint64_t>(os_transparent_management.next_interval_cycle) + 1;
            os_transparent_management.check_interval_swap(swap_idle, not_in_warmup);

            THEN("Nothing is migrated")
            {
                REQUIRE(os_transparent_management.remapping_request_queue.empty());
                REQUIRE(translate(os_transparent_management, fast_memory_address) == fast_memory_address);
            }
        }
    }
}

#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT, IDEAL_SINGLE_MEMPOD */

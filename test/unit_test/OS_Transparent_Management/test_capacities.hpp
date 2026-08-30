#ifndef OS_TRANSPARENT_MANAGEMENT_TEST_CAPACITIES_HPP
#define OS_TRANSPARENT_MANAGEMENT_TEST_CAPACITIES_HPP

#include <cstdint>

#include "ChampSim/util/bits.h" // DATA_MANAGEMENT_OFFSET_BITS expands to champsim::lg2()
#include "ProjectConfiguration.h" // User file

/**
 * @brief The hybrid memory the tests in this directory build their design under test with.
 *
 * @note
 * Both capacities must be powers of two: a design derives fast_memory_offset_bit from
 * champsim::lg2(fast_memory_max_address). They are kept small so a test can walk a whole
 * congruence group, yet large enough that every proposal's data management granularity
 * (64 B for CAMEO, 2 KiB for MemPod, 4 KiB for variable granularity) divides them, and
 * that total / fast stays within a congruence group's member count.
 */
namespace otm_test
{
inline constexpr uint64_t fast_memory_capacity  = 1 * MiB;
inline constexpr uint64_t total_capacity        = 4 * MiB;

/** @brief Number of members in a congruence group, that is total capacity over fast memory capacity */
inline constexpr uint64_t congruence_group_size = total_capacity / fast_memory_capacity;

/** @brief The first physical address of data block `block_index` at the compiled granularity */
inline constexpr uint64_t data_block_address(uint64_t block_index)
{
    return block_index << DATA_MANAGEMENT_OFFSET_BITS;
}

/** @brief Number of data blocks that fit in fast memory */
inline constexpr uint64_t fast_memory_data_blocks = fast_memory_capacity >> DATA_MANAGEMENT_OFFSET_BITS;

/**
 * @brief The physical address of the member of congruence group `group` that lives in the
 *        memory tier `location` (0 is fast memory, 1 and up are slow memory).
 */
inline constexpr uint64_t congruence_group_member_address(uint64_t group, uint64_t location)
{
    return data_block_address(location * fast_memory_data_blocks + group);
}
} // namespace otm_test

#endif /* OS_TRANSPARENT_MANAGEMENT_TEST_CAPACITIES_HPP */

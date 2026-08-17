#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/champsim_constants.h"

/**
 * Upstream defines NUM_CPUS / BLOCK_SIZE / PAGE_SIZE here, because upstream's copies are extern
 * constants supplied by the generated environment that a test build deliberately leaves out.
 * This fork instead defines them as constexpr in ChampSim/champsim_constants.h, so defining them
 * again would collide. The assertion below pins the values the ported tests were written against:
 * they assume a single core, 64 B cache lines and 4 KiB pages.
 */
static_assert(NUM_CPUS == 1, "The unit tests assume a single-core configuration (CPU_USE_MULTIPLE_CORES must be DISABLE)");
static_assert(BLOCK_SIZE == 64, "The unit tests assume 64-byte cache lines");
static_assert(PAGE_SIZE == 4096, "The unit tests assume 4 KiB pages");
#else
#include "champsim.h"
const std::size_t NUM_CPUS = 1;

const unsigned BLOCK_SIZE = 64;
const unsigned PAGE_SIZE = 4096;
#endif /* USER_CODES */

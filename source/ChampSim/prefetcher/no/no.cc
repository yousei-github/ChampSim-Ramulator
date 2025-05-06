#include "ChampSim/prefetcher/no/no.h"

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

uint32_t no::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in)
{
    // assert(addr == ip); // Invariant for instruction prefetchers
    return metadata_in;
}

uint32_t no::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

// uint32_t CACHE::pref_prefetcherDno_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)

// uint32_t CACHE::pref_prefetcherDno_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)

#else

uint32_t no::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
    uint32_t metadata_in)
{
    // assert(addr == ip); // Invariant for instruction prefetchers
    return metadata_in;
}

uint32_t no::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

#endif // USER_CODES

#include <cassert>

#include "ChampSim/cache.h"

#if (USER_CODES == ENABLE)

void CACHE::ipref_prefetcherDno_instr_prefetcher_initialize() {}

void CACHE::ipref_prefetcherDno_instr_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t CACHE::ipref_prefetcherDno_instr_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    assert(addr == ip); // Invariant for instruction prefetchers
    return metadata_in;
}

void CACHE::ipref_prefetcherDno_instr_prefetcher_cycle_operate() {}

uint32_t CACHE::ipref_prefetcherDno_instr_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

void CACHE::ipref_prefetcherDno_instr_prefetcher_final_stats() {}

#else

void CACHE::prefetcher_initialize() {}

void CACHE::prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    assert(addr == ip); // Invariant for instruction prefetchers
    return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

void CACHE::prefetcher_final_stats() {}

#endif // USER_CODES

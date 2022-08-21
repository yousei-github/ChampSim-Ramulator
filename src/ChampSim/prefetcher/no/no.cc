#include "cache.h"

#if USER_CODES == ENABLE
#if PREFETCHER_USE_NO_AND_NO_INSTR == ENABLE
void CACHE::pref_pprefetcherDno_initialize() {}

uint32_t CACHE::pref_pprefetcherDno_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) { return metadata_in; }

uint32_t CACHE::pref_pprefetcherDno_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::pref_pprefetcherDno_cycle_operate() {}

void CACHE::pref_pprefetcherDno_final_stats() {}

#endif

#else
void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) { return metadata_in; }

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}
#endif

#include "ChampSim/cache.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

void CACHE::pref_prefetcherDnext_line_prefetcher_initialize()
{
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(output_statistics.file_handler, "%s Next line prefetcher\n", NAME.c_str());
#else
    std::cout << NAME << " Next line prefetcher" << std::endl;
#endif
}

uint32_t CACHE::pref_prefetcherDnext_line_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    uint64_t pf_addr = addr + (1 << LOG2_BLOCK_SIZE);
    prefetch_line(pf_addr, true, metadata_in);
    return metadata_in;
}

uint32_t CACHE::pref_prefetcherDnext_line_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

void CACHE::pref_prefetcherDnext_line_prefetcher_cycle_operate() {}

void CACHE::pref_prefetcherDnext_line_prefetcher_final_stats() {}

#else

void CACHE::prefetcher_initialize() {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    uint64_t pf_addr = addr + (1 << LOG2_BLOCK_SIZE);
    prefetch_line(pf_addr, true, metadata_in);
    return metadata_in;
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {}

void CACHE::prefetcher_final_stats() {}

#endif // USER_CODES

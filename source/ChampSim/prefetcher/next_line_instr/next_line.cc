#include "ChampSim/cache.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

void CACHE::ipref_prefetcherDnext_line_instr_prefetcher_initialize()
{
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(output_statistics.file_handler, "%s Next line instruction prefetcher\n", NAME.c_str());
#else
    std::cout << NAME << " Next line instruction prefetcher" << std::endl;
#endif
}

void CACHE::ipref_prefetcherDnext_line_instr_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t CACHE::ipref_prefetcherDnext_line_instr_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    // assert(addr == ip); // Invariant for instruction prefetchers
    uint64_t pf_addr = addr + (1 << LOG2_BLOCK_SIZE);
    prefetch_line(pf_addr, true, metadata_in);
    return metadata_in;
}

uint32_t CACHE::ipref_prefetcherDnext_line_instr_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

void CACHE::ipref_prefetcherDnext_line_instr_prefetcher_cycle_operate() {}

void CACHE::ipref_prefetcherDnext_line_instr_prefetcher_final_stats() {}

#else

void CACHE::prefetcher_initialize() {}

void CACHE::prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    // assert(addr == ip); // Invariant for instruction prefetchers
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

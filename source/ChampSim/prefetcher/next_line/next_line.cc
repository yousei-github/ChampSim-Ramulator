#include "ChampSim/prefetcher/next_line/next_line.h"

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

uint32_t next_line::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in)
{
    champsim::block_number pf_addr {addr};
    prefetch_line(champsim::address {pf_addr + 1}, true, metadata_in);
    return metadata_in;
}

uint32_t next_line::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

// void CACHE::pref_prefetcherDnext_line_prefetcher_initialize()
// {
// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//     fprintf(output_statistics.file_handler, "%s Next line prefetcher\n", NAME.c_str());
// #else
//     std::cout << NAME << " Next line prefetcher" << std::endl;
// #endif
// }

// uint32_t CACHE::pref_prefetcherDnext_line_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)

// uint32_t CACHE::pref_prefetcherDnext_line_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)

#else

uint32_t next_line::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
    uint32_t metadata_in)
{
    champsim::block_number pf_addr {addr};
    prefetch_line(champsim::address {pf_addr + 1}, true, metadata_in);
    return metadata_in;
}

uint32_t next_line::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in)
{
    return metadata_in;
}

#endif // USER_CODES

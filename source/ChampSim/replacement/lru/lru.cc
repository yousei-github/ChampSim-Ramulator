#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

#include "ChampSim/cache.h"

#if (USER_CODES == ENABLE)

namespace
{
std::map<CACHE*, std::vector<uint64_t>> last_used_cycles;
} // namespace

void CACHE::repl_replacementDlru_initialize_replacement()
{
    ::last_used_cycles[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY);
}

uint32_t CACHE::repl_replacementDlru_find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    auto begin  = std::next(std::begin(::last_used_cycles[this]), set * NUM_WAY);
    auto end    = std::next(begin, NUM_WAY);

    // Find the way whose last use cycle is most distant
    auto victim = std::min_element(begin, end);
    assert(begin <= victim);
    assert(victim < end);
    return static_cast<uint32_t>(std::distance(begin, victim)); // cast protected by prior asserts
}

void CACHE::repl_replacementDlru_update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    // Mark the way as being used on the current cycle
    if (! hit || access_type {type} != access_type::WRITE) // Skip this for writeback hits
        ::last_used_cycles[this].at(set * NUM_WAY + way) = current_cycle;
}

void CACHE::repl_replacementDlru_replacement_final_stats() {}

#else

namespace
{
std::map<CACHE*, std::vector<uint64_t>> last_used_cycles;
} // namespace

void CACHE::initialize_replacement() { ::last_used_cycles[this] = std::vector<uint64_t>(NUM_SET * NUM_WAY); }

uint32_t CACHE::find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    auto begin  = std::next(std::begin(::last_used_cycles[this]), set * NUM_WAY);
    auto end    = std::next(begin, NUM_WAY);

    // Find the way whose last use cycle is most distant
    auto victim = std::min_element(begin, end);
    assert(begin <= victim);
    assert(victim < end);
    return static_cast<uint32_t>(std::distance(begin, victim)); // cast protected by prior asserts
}

void CACHE::update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
    uint8_t hit)
{
    // Mark the way as being used on the current cycle
    if (! hit || access_type {type} != access_type::WRITE) // Skip this for writeback hits
        ::last_used_cycles[this].at(set * NUM_WAY + way) = current_cycle;
}

void CACHE::replacement_final_stats() {}

#endif // USER_CODES

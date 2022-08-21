#include "ooo_cpu.h"

#if USER_CODES == ENABLE
#if PREFETCHER_USE_NO_AND_NO_INSTR == ENABLE
void O3_CPU::pref_pprefetcherDno_instr_initialize() {}

void O3_CPU::pref_pprefetcherDno_instr_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t O3_CPU::pref_pprefetcherDno_instr_cache_operate(uint64_t v_addr, uint8_t cache_hit, uint8_t prefetch_hit, uint32_t metadata_in) { return metadata_in; }

void O3_CPU::pref_pprefetcherDno_instr_cycle_operate() {}

uint32_t O3_CPU::pref_pprefetcherDno_instr_cache_fill(uint64_t v_addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_v_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void O3_CPU::pref_pprefetcherDno_instr_final_stats() {}

#endif

#else
void O3_CPU::prefetcher_initialize() {}

void O3_CPU::prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target) {}

uint32_t O3_CPU::prefetcher_cache_operate(uint64_t v_addr, uint8_t cache_hit, uint8_t prefetch_hit, uint32_t metadata_in) { return metadata_in; }

void O3_CPU::prefetcher_cycle_operate() {}

uint32_t O3_CPU::prefetcher_cache_fill(uint64_t v_addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_v_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void O3_CPU::prefetcher_final_stats() {}
#endif

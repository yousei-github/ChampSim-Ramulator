enum class repl_t
{
    rreplacementDlru
};

void repl_rreplacementDlru_initialize();
void impl_replacement_initialize()
{
    if (repl_type == repl_t::rreplacementDlru) return repl_rreplacementDlru_initialize();
    throw std::invalid_argument("Replacement policy module not found");
}

uint32_t repl_rreplacementDlru_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
uint32_t impl_replacement_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    if (repl_type == repl_t::rreplacementDlru) return repl_rreplacementDlru_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    throw std::invalid_argument("Replacement policy module not found");
}

void repl_rreplacementDlru_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
void impl_replacement_update_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    if (repl_type == repl_t::rreplacementDlru) return repl_rreplacementDlru_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    throw std::invalid_argument("Replacement policy module not found");
}

void repl_rreplacementDlru_final_stats();
void impl_replacement_final_stats()
{
    if (repl_type == repl_t::rreplacementDlru) return repl_rreplacementDlru_final_stats();
    throw std::invalid_argument("Replacement policy module not found");
}

enum class pref_t
{
    pprefetcherDno,
    CPU_REDIRECT_pprefetcherDno_instr_
};

void pref_pprefetcherDno_initialize();
void impl_prefetcher_initialize()
{
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_initialize();
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_initialize();
    throw std::invalid_argument("Data prefetcher module not found");
}

uint32_t pref_pprefetcherDno_cache_operate(uint64_t, uint64_t, uint8_t, uint8_t, uint32_t);
uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
{
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_cache_operate(addr, ip, cache_hit, type, metadata_in);
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_operate(addr, cache_hit, (type == PREFETCH), metadata_in);
    throw std::invalid_argument("Data prefetcher module not found");
}

uint32_t pref_pprefetcherDno_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    throw std::invalid_argument("Data prefetcher module not found");
}

void pref_pprefetcherDno_cycle_operate();
void impl_prefetcher_cycle_operate()
{
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_cycle_operate();
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cycle_operate();
    throw std::invalid_argument("Data prefetcher module not found");
}

void pref_pprefetcherDno_final_stats();
void impl_prefetcher_final_stats()
{
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_final_stats();
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_final_stats();
    throw std::invalid_argument("Data prefetcher module not found");
}


#ifndef CACHE_H
#define CACHE_H

#include <functional>
#include <list>
#include <string>
#include <vector>

#include "champsim.h"
#include "delay_queue.hpp"
#include "memory_class.h"
#include "ooo_cpu.h"
#include "operable.h"

#include "ProjectConfiguration.h" // user file

// virtual address space prefetching
#define VA_PREFETCH_TRANSLATION_LATENCY 2

#if (USER_CODES == ENABLE)
#include <array>
#include "ooo_cpu.h"
#include "vmem.h"

class CACHE : public champsim::operable, public MemoryRequestConsumer, public MemoryRequestProducer
{
public:
#if (USER_CODES) == (ENABLE)
  uint32_t cpu = 0;
#else
  uint32_t cpu;
#endif

  const std::string NAME;
  const uint32_t NUM_SET, NUM_WAY, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
  const uint32_t HIT_LATENCY, FILL_LATENCY, OFFSET_BITS;
  std::vector<BLOCK> block{NUM_SET * NUM_WAY};
  const uint32_t MAX_READ, MAX_WRITE;
  uint32_t reads_available_this_cycle, writes_available_this_cycle;
  const bool prefetch_as_load;
  const bool match_offset_bits;
  const bool virtual_prefetch;
  bool ever_seen_data = false;
  const unsigned pref_activate_mask = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH));

  // prefetch stats
  uint64_t pf_requested = 0, pf_issued = 0, pf_useful = 0, pf_useless = 0, pf_fill = 0;

  // queues
  champsim::delay_queue<PACKET> RQ{RQ_SIZE, HIT_LATENCY}, // read queue
    PQ{PQ_SIZE, HIT_LATENCY},                             // prefetch queue
    VAPQ{PQ_SIZE, VA_PREFETCH_TRANSLATION_LATENCY},       // virtual address prefetch queue
    WQ{WQ_SIZE, HIT_LATENCY};                             // write queue

#if (USER_CODES) == (ENABLE)
  // Miss status holding registers (MSHR) is the hardware structure for tracking outstanding misses.
  // This mechanism is to insure accesses to the same location execute in program order.
  // Each MSHR refers to one missing cache line and contains a valid bit, the tag of the cache line and
  // one or more subentries to handle multiple misses to the same cache line. (from google by "cache mshr is")
#endif
  std::list<PACKET> MSHR; // MSHR

  uint64_t sim_access[NUM_CPUS][NUM_TYPES] = {}, sim_hit[NUM_CPUS][NUM_TYPES] = {}, sim_miss[NUM_CPUS][NUM_TYPES] = {}, roi_access[NUM_CPUS][NUM_TYPES] = {},
    roi_hit[NUM_CPUS][NUM_TYPES] = {}, roi_miss[NUM_CPUS][NUM_TYPES] = {};

  uint64_t RQ_ACCESS = 0, RQ_MERGED = 0, RQ_FULL = 0, RQ_TO_CACHE = 0, PQ_ACCESS = 0, PQ_MERGED = 0, PQ_FULL = 0, PQ_TO_CACHE = 0, WQ_ACCESS = 0, WQ_MERGED = 0,
    WQ_FULL = 0, WQ_FORWARD = 0, WQ_TO_CACHE = 0;

  uint64_t total_miss_latency = 0;

  // functions
  int add_rq(PACKET* packet) override;
  int add_wq(PACKET* packet) override;
  int add_pq(PACKET* packet) override;

  void return_data(PACKET* packet) override;
  void operate() override;
  void operate_writes();
  void operate_reads();

  uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;
  uint32_t get_size(uint8_t queue_type, uint64_t address) override;

  uint32_t get_set(uint64_t address);
  uint32_t get_way(uint64_t address, uint32_t set);

  int invalidate_entry(uint64_t inval_addr);
  int prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata);
  int prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata); // deprecated

  void add_mshr(PACKET* packet);
  void va_translate_prefetches();

#if USER_CODES == ENABLE
  // handle_fill() handles MSHR.
  // handle_writeback() handles WQ.
  // handle_read() handles RQ.
  // handle_prefetch() handles PQ.
#endif
  void handle_fill();
  void handle_writeback();
  void handle_read();
  void handle_prefetch();

#if USER_CODES == ENABLE
  // readlike_hit() reads a cache line.
  // readlike_miss() reads a cache line from lower level, add new entry in MSHR.
  // filllike_miss() store a new cache line.
#endif
  void readlike_hit(std::size_t set, std::size_t way, PACKET& handle_pkt);
  bool readlike_miss(PACKET& handle_pkt);
  bool filllike_miss(std::size_t set, std::size_t way, PACKET& handle_pkt);

  bool should_activate_prefetcher(int type);

  void print_deadlock() override;

#if (USER_CODES) == (ENABLE)
  /* Definition and declaration for replacement policy */
  // Replacement policy type selection, i.e., lru, ship, srrip, drrip.
  enum class repl_t { rreplacementDlru, rreplacementDship, rreplacementDsrrip, rreplacementDdrrip };

  // Initialization for replacement policy
  void repl_rreplacementDlru_initialize();
  void repl_rreplacementDship_initialize();
  void repl_rreplacementDsrrip_initialize();
  void repl_rreplacementDdrrip_initialize();

  void impl_replacement_initialize()
  {
    if (repl_type == repl_t::rreplacementDlru) repl_rreplacementDlru_initialize();
    else if (repl_type == repl_t::rreplacementDship) repl_rreplacementDship_initialize();
    else if (repl_type == repl_t::rreplacementDsrrip) repl_rreplacementDsrrip_initialize();
    else if (repl_type == repl_t::rreplacementDdrrip) repl_rreplacementDdrrip_initialize();
    else
    {
      std::cout << __func__ << ": Replacement policy module not found." << std::endl;
      throw std::invalid_argument("Replacement policy module not found");
    }
  }

  // Victim for replacement policy
  uint32_t repl_rreplacementDlru_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
  uint32_t repl_rreplacementDship_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
  uint32_t repl_rreplacementDsrrip_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
  uint32_t repl_rreplacementDdrrip_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);

  uint32_t impl_replacement_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
  {
    if (repl_type == repl_t::rreplacementDlru) return repl_rreplacementDlru_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    else if (repl_type == repl_t::rreplacementDship) return repl_rreplacementDship_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    else if (repl_type == repl_t::rreplacementDsrrip) return repl_rreplacementDsrrip_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    else if (repl_type == repl_t::rreplacementDdrrip) return repl_rreplacementDdrrip_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    else
    {
      std::cout << __func__ << ": Replacement policy module not found." << std::endl;
      throw std::invalid_argument("Replacement policy module not found");
    }
  }

  // Update for replacement policy
  void repl_rreplacementDlru_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
  void repl_rreplacementDship_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
  void repl_rreplacementDsrrip_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
  void repl_rreplacementDdrrip_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);

  void impl_replacement_update_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
  {
    if (repl_type == repl_t::rreplacementDlru) repl_rreplacementDlru_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    else if (repl_type == repl_t::rreplacementDship) repl_rreplacementDship_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    else if (repl_type == repl_t::rreplacementDsrrip) repl_rreplacementDsrrip_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    else if (repl_type == repl_t::rreplacementDdrrip) repl_rreplacementDdrrip_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    else
    {
      std::cout << __func__ << ": Replacement policy module not found." << std::endl;
      throw std::invalid_argument("Replacement policy module not found");
    }
  }

  // Final stats for replacement policy
  void repl_rreplacementDlru_final_stats();
  void repl_rreplacementDship_final_stats();
  void repl_rreplacementDsrrip_final_stats();
  void repl_rreplacementDdrrip_final_stats();

  void impl_replacement_final_stats()
  {
    if (repl_type == repl_t::rreplacementDlru) repl_rreplacementDlru_final_stats();
    else if (repl_type == repl_t::rreplacementDship) repl_rreplacementDship_final_stats();
    else if (repl_type == repl_t::rreplacementDsrrip) repl_rreplacementDsrrip_final_stats();
    else if (repl_type == repl_t::rreplacementDdrrip) repl_rreplacementDdrrip_final_stats();
    else
    {
      std::cout << __func__ << ": Replacement policy module not found." << std::endl;
      throw std::invalid_argument("Replacement policy module not found");
    }
  }

  /* Definition and declaration for data prefetcher */
  // Data prefetcher type selection, i.e., no, next_line, ip_stride, no_instr, next_line_instr.
  enum class pref_t { pprefetcherDno, pprefetcherDnext_line, pprefetcherDip_stride, CPU_REDIRECT_pprefetcherDno_instr_, CPU_REDIRECT_pprefetcherDnext_line_instr_ };

  // Initialization for data prefetcher
  void pref_pprefetcherDno_initialize();
  void pref_pprefetcherDnext_line_initialize();
  void pref_pprefetcherDip_stride_initialize();

  void impl_prefetcher_initialize()
  {
    if (pref_type == pref_t::pprefetcherDno) pref_pprefetcherDno_initialize();
    else if (pref_type == pref_t::pprefetcherDnext_line) pref_pprefetcherDnext_line_initialize();
    else if (pref_type == pref_t::pprefetcherDip_stride) pref_pprefetcherDip_stride_initialize();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) ooo_cpu[cpu]->pref_pprefetcherDno_instr_initialize();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_) ooo_cpu[cpu]->pref_pprefetcherDnext_line_instr_initialize();
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

  // Cache operate for data prefetcher
  uint32_t pref_pprefetcherDno_cache_operate(uint64_t, uint64_t, uint8_t, uint8_t, uint32_t);
  uint32_t pref_pprefetcherDnext_line_cache_operate(uint64_t, uint64_t, uint8_t, uint8_t, uint32_t);
  uint32_t pref_pprefetcherDip_stride_cache_operate(uint64_t, uint64_t, uint8_t, uint8_t, uint32_t);

  uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
  {
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_cache_operate(addr, ip, cache_hit, type, metadata_in);
    else if (pref_type == pref_t::pprefetcherDnext_line) return pref_pprefetcherDnext_line_cache_operate(addr, ip, cache_hit, type, metadata_in);
    else if (pref_type == pref_t::pprefetcherDip_stride) return pref_pprefetcherDip_stride_cache_operate(addr, ip, cache_hit, type, metadata_in);
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_operate(addr, cache_hit, (type == PREFETCH), metadata_in);
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_) return ooo_cpu[cpu]->pref_pprefetcherDnext_line_instr_cache_operate(addr, cache_hit, (type == PREFETCH), metadata_in);
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

  // Cache fill for data prefetcher
  uint32_t pref_pprefetcherDno_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
  uint32_t pref_pprefetcherDnext_line_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
  uint32_t pref_pprefetcherDip_stride_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);

  uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
  {
    if (pref_type == pref_t::pprefetcherDno) return pref_pprefetcherDno_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    else if (pref_type == pref_t::pprefetcherDnext_line) return pref_pprefetcherDnext_line_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    else if (pref_type == pref_t::pprefetcherDip_stride) return pref_pprefetcherDip_stride_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_) return ooo_cpu[cpu]->pref_pprefetcherDnext_line_instr_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

  // Cycle operate for data prefetcher
  void pref_pprefetcherDno_cycle_operate();
  void pref_pprefetcherDnext_line_cycle_operate();
  void pref_pprefetcherDip_stride_cycle_operate();

  void impl_prefetcher_cycle_operate()
  {
    if (pref_type == pref_t::pprefetcherDno) pref_pprefetcherDno_cycle_operate();
    else if (pref_type == pref_t::pprefetcherDnext_line) pref_pprefetcherDnext_line_cycle_operate();
    else if (pref_type == pref_t::pprefetcherDip_stride) pref_pprefetcherDip_stride_cycle_operate();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) ooo_cpu[cpu]->pref_pprefetcherDno_instr_cycle_operate();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_) ooo_cpu[cpu]->pref_pprefetcherDnext_line_instr_cycle_operate();
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

  // Final_stats for data prefetcher
  void pref_pprefetcherDno_final_stats();
  void pref_pprefetcherDnext_line_final_stats();
  void pref_pprefetcherDip_stride_final_stats();

  void impl_prefetcher_final_stats()
  {
    if (pref_type == pref_t::pprefetcherDno) pref_pprefetcherDno_final_stats();
    else if (pref_type == pref_t::pprefetcherDnext_line) pref_pprefetcherDnext_line_final_stats();
    else if (pref_type == pref_t::pprefetcherDip_stride) pref_pprefetcherDip_stride_final_stats();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_) ooo_cpu[cpu]->pref_pprefetcherDno_instr_final_stats();
    else if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDnext_line_instr_) ooo_cpu[cpu]->pref_pprefetcherDnext_line_instr_final_stats();
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

#else
#include "cache_modules.inc"
#endif  // USER_CODES

  const repl_t repl_type;
  const pref_t pref_type;

  std::array<O3_CPU*, NUM_CPUS>& ooo_cpu;
  VirtualMemory& vmem;

  // constructor
  CACHE(std::string v1, double freq_scale, unsigned fill_level, uint32_t v2, int v3, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8, uint32_t hit_lat,
        uint32_t fill_lat, uint32_t max_read, uint32_t max_write, std::size_t offset_bits, bool pref_load, bool wq_full_addr, bool va_pref,
        unsigned pref_act_mask, MemoryRequestConsumer* ll, pref_t pref, repl_t repl, std::array<O3_CPU*, NUM_CPUS>& ooo_cpu, VirtualMemory& vmem)
    : champsim::operable(freq_scale), MemoryRequestConsumer(fill_level), MemoryRequestProducer(ll), NAME(v1), NUM_SET(v2), NUM_WAY(v3), WQ_SIZE(v5),
    RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8), HIT_LATENCY(hit_lat), FILL_LATENCY(fill_lat), OFFSET_BITS(offset_bits), MAX_READ(max_read),
    MAX_WRITE(max_write), prefetch_as_load(pref_load), match_offset_bits(wq_full_addr), virtual_prefetch(va_pref), pref_activate_mask(pref_act_mask),
    repl_type(repl), pref_type(pref), ooo_cpu(ooo_cpu), vmem(vmem)
  {
  }
};

#else
extern std::array<O3_CPU*, NUM_CPUS> ooo_cpu;

class CACHE : public champsim::operable, public MemoryRequestConsumer, public MemoryRequestProducer
{
public:

  uint32_t cpu;

  const std::string NAME;
  const uint32_t NUM_SET, NUM_WAY, WQ_SIZE, RQ_SIZE, PQ_SIZE, MSHR_SIZE;
  const uint32_t HIT_LATENCY, FILL_LATENCY, OFFSET_BITS;
  std::vector<BLOCK> block{NUM_SET * NUM_WAY};
  const uint32_t MAX_READ, MAX_WRITE;
  uint32_t reads_available_this_cycle, writes_available_this_cycle;
  const bool prefetch_as_load;
  const bool match_offset_bits;
  const bool virtual_prefetch;
  bool ever_seen_data = false;
  const unsigned pref_activate_mask = (1 << static_cast<int>(LOAD)) | (1 << static_cast<int>(PREFETCH));

  // prefetch stats
  uint64_t pf_requested = 0, pf_issued = 0, pf_useful = 0, pf_useless = 0, pf_fill = 0;

  // queues
  champsim::delay_queue<PACKET> RQ{RQ_SIZE, HIT_LATENCY}, // read queue
    PQ{PQ_SIZE, HIT_LATENCY},                             // prefetch queue
    VAPQ{PQ_SIZE, VA_PREFETCH_TRANSLATION_LATENCY},       // virtual address prefetch queue
    WQ{WQ_SIZE, HIT_LATENCY};                             // write queue

#if USER_CODES == ENABLE
  // Miss status holding registers (MSHR) is the hardware structure for tracking outstanding misses.
  // This mechanism is to insure accesses to the same location execute in program order.
  // Each MSHR refers to one missing cache line and contains a valid bit, the tag of the cache line and
  // one or more subentries to handle multiple misses to the same cache line. (from google by "cache mshr is")
#endif
  std::list<PACKET> MSHR; // MSHR

  uint64_t sim_access[NUM_CPUS][NUM_TYPES] = {}, sim_hit[NUM_CPUS][NUM_TYPES] = {}, sim_miss[NUM_CPUS][NUM_TYPES] = {}, roi_access[NUM_CPUS][NUM_TYPES] = {},
    roi_hit[NUM_CPUS][NUM_TYPES] = {}, roi_miss[NUM_CPUS][NUM_TYPES] = {};

  uint64_t RQ_ACCESS = 0, RQ_MERGED = 0, RQ_FULL = 0, RQ_TO_CACHE = 0, PQ_ACCESS = 0, PQ_MERGED = 0, PQ_FULL = 0, PQ_TO_CACHE = 0, WQ_ACCESS = 0, WQ_MERGED = 0,
    WQ_FULL = 0, WQ_FORWARD = 0, WQ_TO_CACHE = 0;

  uint64_t total_miss_latency = 0;

  // functions
  int add_rq(PACKET* packet) override;
  int add_wq(PACKET* packet) override;
  int add_pq(PACKET* packet) override;

  void return_data(PACKET* packet) override;
  void operate() override;
  void operate_writes();
  void operate_reads();

  uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;
  uint32_t get_size(uint8_t queue_type, uint64_t address) override;

  uint32_t get_set(uint64_t address);
  uint32_t get_way(uint64_t address, uint32_t set);

  int invalidate_entry(uint64_t inval_addr);
  int prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata);
  int prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata); // deprecated

  void add_mshr(PACKET* packet);
  void va_translate_prefetches();

#if USER_CODES == ENABLE
  // handle_fill() handles MSHR.
  // handle_writeback() handles WQ.
  // handle_read() handles RQ.
  // handle_prefetch() handles PQ.
#endif
  void handle_fill();
  void handle_writeback();
  void handle_read();
  void handle_prefetch();

#if USER_CODES == ENABLE
  // readlike_hit() reads a cache line.
  // readlike_miss() reads a cache line from lower level, add new entry in MSHR.
  // filllike_miss() store a new cache line.
#endif
  void readlike_hit(std::size_t set, std::size_t way, PACKET& handle_pkt);
  bool readlike_miss(PACKET& handle_pkt);
  bool filllike_miss(std::size_t set, std::size_t way, PACKET& handle_pkt);

  bool should_activate_prefetcher(int type);

  void print_deadlock() override;

#if USER_CODES == ENABLE
  enum class repl_t { rreplacementDlru }; // Replacement policy type selection

  void repl_rreplacementDlru_initialize();
  void impl_replacement_initialize()
  {
    if (repl_type == repl_t::rreplacementDlru)
      repl_rreplacementDlru_initialize();
    else
    {
      std::cout << __func__ << ": Replacement policy module not found." << std::endl;
      throw std::invalid_argument("Replacement policy module not found");
    }
  }

  uint32_t repl_rreplacementDlru_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
  uint32_t impl_replacement_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
  {
    if (repl_type == repl_t::rreplacementDlru)
      return repl_rreplacementDlru_victim(cpu, instr_id, set, current_set, ip, full_addr, type);
    throw std::invalid_argument("Replacement policy module not found");
  }

  void repl_rreplacementDlru_update(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
  void impl_replacement_update_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
                                     uint8_t hit)
  {
    if (repl_type == repl_t::rreplacementDlru)
      return repl_rreplacementDlru_update(cpu, set, way, full_addr, ip, victim_addr, type, hit);
    throw std::invalid_argument("Replacement policy module not found");
  }

  void repl_rreplacementDlru_final_stats();
  void impl_replacement_final_stats()
  {
    if (repl_type == repl_t::rreplacementDlru)
      return repl_rreplacementDlru_final_stats();
    throw std::invalid_argument("Replacement policy module not found");
  }

  enum class pref_t { pprefetcherDno, CPU_REDIRECT_pprefetcherDno_instr_ }; // Data prefetcher type selection

  void pref_pprefetcherDno_initialize();
  void impl_prefetcher_initialize()
  {
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
      ooo_cpu[cpu]->pref_pprefetcherDno_instr_initialize();
    else if (pref_type == pref_t::pprefetcherDno)
      pref_pprefetcherDno_initialize();
    else
    {
      std::cout << __func__ << ": Data prefetcher module not found." << std::endl;
      throw std::invalid_argument("Data prefetcher module not found");
    }
  }

  uint32_t pref_pprefetcherDno_cache_operate(uint64_t, uint64_t, uint8_t, uint8_t, uint32_t);
  uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in)
  {
    if (pref_type == pref_t::pprefetcherDno)
      return pref_pprefetcherDno_cache_operate(addr, ip, cache_hit, type, metadata_in);
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
      return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_operate(addr, cache_hit, (type == PREFETCH), metadata_in);
    throw std::invalid_argument("Data prefetcher module not found");
  }

  uint32_t pref_pprefetcherDno_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
  uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
  {
    if (pref_type == pref_t::pprefetcherDno)
      return pref_pprefetcherDno_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
      return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    throw std::invalid_argument("Data prefetcher module not found");
  }

  void pref_pprefetcherDno_cycle_operate();
  void impl_prefetcher_cycle_operate()
  {
    if (pref_type == pref_t::pprefetcherDno)
      return pref_pprefetcherDno_cycle_operate();
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
      return ooo_cpu[cpu]->pref_pprefetcherDno_instr_cycle_operate();
    throw std::invalid_argument("Data prefetcher module not found");
  }

  void pref_pprefetcherDno_final_stats();
  void impl_prefetcher_final_stats()
  {
    if (pref_type == pref_t::pprefetcherDno)
      return pref_pprefetcherDno_final_stats();
    if (pref_type == pref_t::CPU_REDIRECT_pprefetcherDno_instr_)
      return ooo_cpu[cpu]->pref_pprefetcherDno_instr_final_stats();
    throw std::invalid_argument("Data prefetcher module not found");
  }

#if PREFETCHER_USE_NO_AND_NO_INSTR == ENABLE

#endif

#if REPLACEMENT_USE_LRU == ENABLE

#endif

#else
#include "cache_modules.inc"
#endif

  const repl_t repl_type;
  const pref_t pref_type;

  // constructor
  CACHE(std::string v1, double freq_scale, unsigned fill_level, uint32_t v2, int v3, uint32_t v5, uint32_t v6, uint32_t v7, uint32_t v8, uint32_t hit_lat,
        uint32_t fill_lat, uint32_t max_read, uint32_t max_write, std::size_t offset_bits, bool pref_load, bool wq_full_addr, bool va_pref,
        unsigned pref_act_mask, MemoryRequestConsumer* ll, pref_t pref, repl_t repl)
    : champsim::operable(freq_scale), MemoryRequestConsumer(fill_level), MemoryRequestProducer(ll), NAME(v1), NUM_SET(v2), NUM_WAY(v3), WQ_SIZE(v5),
    RQ_SIZE(v6), PQ_SIZE(v7), MSHR_SIZE(v8), HIT_LATENCY(hit_lat), FILL_LATENCY(fill_lat), OFFSET_BITS(offset_bits), MAX_READ(max_read),
    MAX_WRITE(max_write), prefetch_as_load(pref_load), match_offset_bits(wq_full_addr), virtual_prefetch(va_pref), pref_activate_mask(pref_act_mask),
    repl_type(repl), pref_type(pref)
  {
  }
};
#endif  // USER_CODES

#endif

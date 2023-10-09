/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CHAMPSIM_MODULE
#define SET_ASIDE_CHAMPSIM_MODULE
#undef CHAMPSIM_MODULE
#endif

#ifndef CACHE_H
#define CACHE_H

#include <array>
#include <bitset>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/channel.h"
#include "ChampSim/module_impl.h"
#include "ChampSim/operable.h"
#include "ProjectConfiguration.h" // User file

struct cache_stats
{
    std::string name;
    // prefetch stats
    uint64_t pf_requested                                                                              = 0;
    uint64_t pf_issued                                                                                 = 0;
    uint64_t pf_useful                                                                                 = 0;
    uint64_t pf_useless                                                                                = 0;
    uint64_t pf_fill                                                                                   = 0;

    std::array<std::array<uint64_t, NUM_CPUS>, champsim::to_underlying(access_type::NUM_TYPES)> hits   = {};
    std::array<std::array<uint64_t, NUM_CPUS>, champsim::to_underlying(access_type::NUM_TYPES)> misses = {};

    double avg_miss_latency                                                                            = 0;
    uint64_t total_miss_latency                                                                        = 0;
};

class CACHE : public champsim::operable
{
    enum [[deprecated(
        "Prefetchers may not specify arbitrary fill levels. Use CACHE::prefetch_line(pf_addr, fill_this_level, prefetch_metadata) instead.")]] FILL_LEVEL {
        FILL_L1 = 1, FILL_L2 = 2, FILL_LLC = 4, FILL_DRC = 8, FILL_DRAM = 16};

    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;

    struct tag_lookup_type
    {
        uint64_t address;
        uint64_t v_address;
        uint64_t data;
        uint64_t ip;
        uint64_t instr_id;

        uint32_t pf_metadata;
        uint32_t cpu;

        access_type type;
        bool prefetch_from_this;
        bool skip_fill;
        bool is_translated;
        bool translate_issued = false;

        uint8_t asid[2]       = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};

        uint64_t event_cycle  = std::numeric_limits<uint64_t>::max();

        std::vector<std::reference_wrapper<ooo_model_instr>> instr_depend_on_me {};
        std::vector<std::deque<response_type>*> to_return {};

        explicit tag_lookup_type(request_type req): tag_lookup_type(req, false, false) {}

        tag_lookup_type(request_type req, bool local_pref, bool skip);
    };

    struct mshr_type
    {
        uint64_t address;
        uint64_t v_address;
        uint64_t data;
        uint64_t ip;
        uint64_t instr_id;

        uint32_t pf_metadata;
        uint32_t cpu;

        access_type type;
        bool prefetch_from_this;

        uint8_t asid[2]      = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};

        uint64_t event_cycle = std::numeric_limits<uint64_t>::max();
        uint64_t cycle_enqueued;

        std::vector<std::reference_wrapper<ooo_model_instr>> instr_depend_on_me {};
        std::vector<std::deque<response_type>*> to_return {};

        mshr_type(tag_lookup_type req, uint64_t cycle);
        static mshr_type merge(mshr_type predecessor, mshr_type successor);
    };

    bool try_hit(const tag_lookup_type& handle_pkt);
    bool handle_fill(const mshr_type& fill_mshr);
    bool handle_miss(const tag_lookup_type& handle_pkt);
    bool handle_write(const tag_lookup_type& handle_pkt);
    void finish_packet(const response_type& packet);
    void finish_translation(const response_type& packet);

    void issue_translation();

    struct BLOCK
    {
        bool valid           = false;
        bool prefetch        = false;
        bool dirty           = false;

        uint64_t address     = 0;
        uint64_t v_address   = 0;
        uint64_t data        = 0;

        uint32_t pf_metadata = 0;

        BLOCK()              = default;
        explicit BLOCK(mshr_type mshr);
    };

    using set_type = std::vector<BLOCK>;

    std::pair<set_type::iterator, set_type::iterator> get_set_span(uint64_t address);
    std::pair<set_type::const_iterator, set_type::const_iterator> get_set_span(uint64_t address) const;
    std::size_t get_set_index(uint64_t address) const;

    template<typename T>
    bool should_activate_prefetcher(const T& pkt) const;

    template<bool>
    auto initiate_tag_check(champsim::channel* ul = nullptr);

    std::deque<tag_lookup_type> internal_PQ {};
    std::deque<tag_lookup_type> inflight_tag_check {};
    std::deque<tag_lookup_type> translation_stash {};

public:
    std::vector<channel_type*> upper_levels;
    channel_type* lower_level;
    channel_type* lower_translate;

    uint32_t cpu = 0;
    const std::string NAME;
    const uint32_t NUM_SET, NUM_WAY, MSHR_SIZE;
    const std::size_t PQ_SIZE;
    const uint64_t HIT_LATENCY, FILL_LATENCY;
    const unsigned OFFSET_BITS;
    set_type block {NUM_SET * NUM_WAY};
    const long int MAX_TAG, MAX_FILL;
    const bool prefetch_as_load;
    const bool match_offset_bits;
    const bool virtual_prefetch;
    bool ever_seen_data               = false;
    const unsigned pref_activate_mask = (1 << champsim::to_underlying(access_type::LOAD)) | (1 << champsim::to_underlying(access_type::PREFETCH));

    using stats_type                  = cache_stats;

    stats_type sim_stats, roi_stats;

#if (USER_CODES == ENABLE)
/** @brief
 * Miss status holding registers (MSHR) is the hardware structure for tracking outstanding misses.
 * This mechanism is to insure accesses to the same location execute in program order.
 * Each MSHR refers to one missing cache line and contains a valid bit, the tag of the cache line and
 * one or more subentries to handle multiple misses to the same cache line. (from google by "cache mshr is")
*/
#endif
    std::deque<mshr_type> MSHR;
    std::deque<mshr_type> inflight_writes;

    long operate() override final;

    void initialize() override final;
    void begin_phase() override final;
    void end_phase(unsigned cpu) override final;

    [[deprecated("get_occupancy() returns 0 for every input except 0 (MSHR). Use get_mshr_occupancy() instead.")]] std::size_t get_occupancy(uint8_t queue_type,
        uint64_t address);
    [[deprecated("get_size() returns 0 for every input except 0 (MSHR). Use get_mshr_size() instead.")]] std::size_t get_size(uint8_t queue_type,
        uint64_t address);

    std::size_t get_mshr_occupancy() const;
    std::size_t get_mshr_size() const;
    double get_mshr_occupancy_ratio() const;

    std::vector<std::size_t> get_rq_occupancy() const;
    std::vector<std::size_t> get_rq_size() const;
    std::vector<double> get_rq_occupancy_ratio() const;

    std::vector<std::size_t> get_wq_occupancy() const;
    std::vector<std::size_t> get_wq_size() const;
    std::vector<double> get_wq_occupancy_ratio() const;

    std::vector<std::size_t> get_pq_occupancy() const;
    std::vector<std::size_t> get_pq_size() const;
    std::vector<double> get_pq_occupancy_ratio() const;

    [[deprecated("Use get_set_index() instead.")]] uint64_t get_set(uint64_t address) const;
    [[deprecated("This function should not be used to access the blocks directly.")]] uint64_t get_way(uint64_t address, uint64_t set) const;

    uint64_t invalidate_entry(uint64_t inval_addr);
    int prefetch_line(uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata);

    [[deprecated("Use CACHE::prefetch_line(pf_addr, fill_this_level, prefetch_metadata) instead.")]] int
    prefetch_line(uint64_t ip, uint64_t base_addr, uint64_t pf_addr, bool fill_this_level, uint32_t prefetch_metadata);

    void print_deadlock() override;

#if (USER_CODES == ENABLE)

    /* Definition and declaration for data and instruction prefetchers */
    // Data and instruction prefetcher type selection, i.e., ip_stride, next_line, next_line_instr, no, no_instr, spp, va_ampm_lite.
    constexpr static unsigned long long pprefetcherDip_stride                               = 1ull << 0;
    constexpr static unsigned long long pprefetcherDnext_line                               = 1ull << 1;
    constexpr static unsigned long long pprefetcherDnext_line_instr                         = 1ull << 2;
    constexpr static unsigned long long pprefetcherDno                                      = 1ull << 3;
    constexpr static unsigned long long pprefetcherDno_instr                                = 1ull << 4;
    constexpr static unsigned long long pprefetcherDspp_dev                                 = 1ull << 5;
    constexpr static unsigned long long pprefetcherDva_ampm_lite                            = 1ull << 6;
    constexpr static unsigned long long ptestDcppDmodulesDprefetcherDaddress_collector      = 1ull << 7;
    constexpr static unsigned long long ptestDcppDmodulesDprefetcherDmetadata_collector     = 1ull << 8;
    constexpr static unsigned long long ptestDcppDmodulesDprefetcherDmetadata_emitter       = 1ull << 9;
    constexpr static unsigned long long ptestDcppDmodulesDprefetcherDprefetch_hit_collector = 1ull << 10;

    /* Definition and declaration for replacement policies */
    // Replacement policy type selection, i.e., drrip, lru, ship, srrip.
    constexpr static unsigned long long rreplacementDdrrip                                  = 1ull << 0;
    constexpr static unsigned long long rreplacementDlru                                    = 1ull << 1;
    constexpr static unsigned long long rreplacementDship                                   = 1ull << 2;
    constexpr static unsigned long long rreplacementDsrrip                                  = 1ull << 3;
    constexpr static unsigned long long rtestDcppDmodulesDreplacementDlru_collect           = 1ull << 4;
    constexpr static unsigned long long rtestDcppDmodulesDreplacementDmock_replacement      = 1ull << 5;

    // Initialization for data and instruction prefetchers
    [[]] void pref_prefetcherDip_stride_prefetcher_initialize();
    [[]] void pref_prefetcherDnext_line_prefetcher_initialize();
    [[]] void ipref_prefetcherDnext_line_instr_prefetcher_initialize();
    [[]] void pref_prefetcherDno_prefetcher_initialize();
    [[]] void ipref_prefetcherDno_instr_prefetcher_initialize();
    [[]] void pref_prefetcherDspp_dev_prefetcher_initialize();
    [[]] void pref_prefetcherDva_ampm_lite_prefetcher_initialize();
    [[]] void pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_initialize();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_initialize();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_initialize();
    [[]] void pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_initialize();

    // Cache operate for data and instruction prefetchers
    [[nodiscard]] uint32_t pref_prefetcherDip_stride_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDnext_line_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t ipref_prefetcherDnext_line_instr_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDno_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t ipref_prefetcherDno_instr_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDspp_dev_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDva_ampm_lite_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cache_operate(uint64_t, uint64_t, uint8_t, bool, uint8_t, uint32_t);

    // Cache fill for data and instruction prefetchers
    [[nodiscard]] uint32_t pref_prefetcherDip_stride_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDnext_line_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t ipref_prefetcherDnext_line_instr_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDno_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t ipref_prefetcherDno_instr_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDspp_dev_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_prefetcherDva_ampm_lite_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cache_fill(uint64_t, uint32_t, uint32_t, uint8_t, uint64_t, uint32_t);

    // Cycle operate for data and instruction prefetchers
    [[]] void pref_prefetcherDip_stride_prefetcher_cycle_operate();
    [[]] void pref_prefetcherDnext_line_prefetcher_cycle_operate();
    [[]] void ipref_prefetcherDnext_line_instr_prefetcher_cycle_operate();
    [[]] void pref_prefetcherDno_prefetcher_cycle_operate();
    [[]] void ipref_prefetcherDno_instr_prefetcher_cycle_operate();
    [[]] void pref_prefetcherDspp_dev_prefetcher_cycle_operate();
    [[]] void pref_prefetcherDva_ampm_lite_prefetcher_cycle_operate();
    [[]] void pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cycle_operate();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cycle_operate();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cycle_operate();
    [[]] void pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cycle_operate();

    // Final_stats for data and instruction prefetchers
    [[]] void pref_prefetcherDip_stride_prefetcher_final_stats();
    [[]] void pref_prefetcherDnext_line_prefetcher_final_stats();
    [[]] void ipref_prefetcherDnext_line_instr_prefetcher_final_stats();
    [[]] void pref_prefetcherDno_prefetcher_final_stats();
    [[]] void ipref_prefetcherDno_instr_prefetcher_final_stats();
    [[]] void pref_prefetcherDspp_dev_prefetcher_final_stats();
    [[]] void pref_prefetcherDva_ampm_lite_prefetcher_final_stats();
    [[]] void pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_final_stats();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_final_stats();
    [[]] void pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_final_stats();
    [[]] void pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_final_stats();

    // Assert data prefetchers do not operate on branches
    [[noreturn]] void pref_prefetcherDip_stride_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_prefetcherDnext_line_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_prefetcherDno_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_prefetcherDspp_dev_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_prefetcherDva_ampm_lite_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[noreturn]] void pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t) { throw std::runtime_error("Not implemented"); }

    [[]] void ipref_prefetcherDnext_line_instr_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t);
    [[]] void ipref_prefetcherDno_instr_prefetcher_branch_operate(uint64_t, uint8_t, uint64_t);

    // Initialization for replacement policies
    [[]] void repl_replacementDdrrip_initialize_replacement();
    [[]] void repl_replacementDlru_initialize_replacement();
    [[]] void repl_replacementDship_initialize_replacement();
    [[]] void repl_replacementDsrrip_initialize_replacement();
    [[]] void repl_testDcppDmodulesDreplacementDlru_collect_initialize_replacement();
    [[]] void repl_testDcppDmodulesDreplacementDmock_replacement_initialize_replacement();

    // Victim for replacement policies
    [[nodiscard]] uint32_t repl_replacementDdrrip_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t repl_replacementDlru_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t repl_replacementDship_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t repl_replacementDsrrip_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t repl_testDcppDmodulesDreplacementDlru_collect_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);
    [[nodiscard]] uint32_t repl_testDcppDmodulesDreplacementDmock_replacement_find_victim(uint32_t, uint64_t, uint32_t, const BLOCK*, uint64_t, uint64_t, uint32_t);

    // Update for replacement policies
    [[]] void repl_replacementDdrrip_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
    [[]] void repl_replacementDlru_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
    [[]] void repl_replacementDship_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
    [[]] void repl_replacementDsrrip_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
    [[]] void repl_testDcppDmodulesDreplacementDlru_collect_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);
    [[]] void repl_testDcppDmodulesDreplacementDmock_replacement_update_replacement_state(uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint8_t);

    // Final stats for replacement policies
    [[]] void repl_replacementDdrrip_replacement_final_stats();
    [[]] void repl_replacementDlru_replacement_final_stats();
    [[]] void repl_replacementDship_replacement_final_stats();
    [[]] void repl_replacementDsrrip_replacement_final_stats();
    [[]] void repl_testDcppDmodulesDreplacementDlru_collect_replacement_final_stats();
    [[]] void repl_testDcppDmodulesDreplacementDmock_replacement_replacement_final_stats();

#else
#include "cache_module_decl.inc"
#endif // USER_CODES

    struct module_concept
    {
        virtual ~module_concept()                                                                                                                               = default;

        virtual void impl_prefetcher_initialize()                                                                                                               = 0;
        virtual uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in) = 0;
        virtual uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)   = 0;
        virtual void impl_prefetcher_cycle_operate()                                                                                                            = 0;
        virtual void impl_prefetcher_final_stats()                                                                                                              = 0;
        virtual void impl_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target)                                                   = 0;

        virtual void impl_initialize_replacement()                                                                                                              = 0;
        virtual uint32_t impl_find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr,
            uint32_t type)                                                                                                                                      = 0;
        virtual void impl_update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr,
            uint32_t type, uint8_t hit)                                                                                                                         = 0;
        virtual void impl_replacement_final_stats()                                                                                                             = 0;
    };

    template<unsigned long long P_FLAG, unsigned long long R_FLAG>
    struct module_model final : module_concept
    {
        CACHE* intern_;

        explicit module_model(CACHE* cache): intern_(cache) {}

        void impl_prefetcher_initialize();
        uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in);
        uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in);
        void impl_prefetcher_cycle_operate();
        void impl_prefetcher_final_stats();
        void impl_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target);

        void impl_initialize_replacement();
        uint32_t impl_find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr,
            uint32_t type);
        void impl_update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr,
            uint32_t type, uint8_t hit);
        void impl_replacement_final_stats();
    };

    std::unique_ptr<module_concept> module_pimpl;

    void impl_prefetcher_initialize() { module_pimpl->impl_prefetcher_initialize(); }

    uint32_t impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
    {
        return module_pimpl->impl_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in);
    }

    uint32_t impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
    {
        return module_pimpl->impl_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in);
    }

    void impl_prefetcher_cycle_operate() { module_pimpl->impl_prefetcher_cycle_operate(); }

    void impl_prefetcher_final_stats() { module_pimpl->impl_prefetcher_final_stats(); }

    void impl_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target)
    {
        module_pimpl->impl_prefetcher_branch_operate(ip, branch_type, branch_target);
    }

    void impl_initialize_replacement() { module_pimpl->impl_initialize_replacement(); }

    uint32_t impl_find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
    {
        return module_pimpl->impl_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type);
    }

    void impl_update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type,
        uint8_t hit)
    {
        module_pimpl->impl_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    }

    void impl_replacement_final_stats() { module_pimpl->impl_replacement_final_stats(); }

    class builder_conversion_tag
    {
    };

    template<unsigned long long P_FLAG = 0, unsigned long long R_FLAG = 0>
    class Builder
    {
        using self_type = Builder<P_FLAG, R_FLAG>;

        std::string m_name {};
        double m_freq_scale {};
        uint32_t m_sets {};
        uint32_t m_ways {};
        std::size_t m_pq_size {std::numeric_limits<std::size_t>::max()};
        uint32_t m_mshr_size {};
        uint64_t m_hit_lat {};
        uint64_t m_fill_lat {};
        uint64_t m_latency {};
        uint32_t m_max_tag {};
        uint32_t m_max_fill {};
        unsigned m_offset_bits {};
        bool m_pref_load {};
        bool m_wq_full_addr {};
        bool m_va_pref {};

        unsigned m_pref_act_mask {};
        std::vector<CACHE::channel_type*> m_uls {};
        CACHE::channel_type* m_ll {};
        CACHE::channel_type* m_lt {nullptr};

        friend class CACHE;

        template<unsigned long long OTHER_P, unsigned long long OTHER_R>
        Builder(builder_conversion_tag, const Builder<OTHER_P, OTHER_R>& other)
        : m_name(other.m_name), m_freq_scale(other.m_freq_scale), m_sets(other.m_sets), m_ways(other.m_ways), m_pq_size(other.m_pq_size),
          m_mshr_size(other.m_mshr_size), m_hit_lat(other.m_hit_lat), m_fill_lat(other.m_fill_lat), m_latency(other.m_latency), m_max_tag(other.m_max_tag),
          m_max_fill(other.m_max_fill), m_offset_bits(other.m_offset_bits), m_pref_load(other.m_pref_load), m_wq_full_addr(other.m_wq_full_addr),
          m_va_pref(other.m_va_pref), m_pref_act_mask(other.m_pref_act_mask), m_uls(other.m_uls), m_ll(other.m_ll), m_lt(other.m_lt)
        {
        }

    public:
        Builder() = default;

        self_type& name(std::string name_)
        {
            m_name = name_;
            return *this;
        }

        self_type& frequency(double freq_scale_)
        {
            m_freq_scale = freq_scale_;
            return *this;
        }

        self_type& sets(uint32_t sets_)
        {
            m_sets = sets_;
            return *this;
        }

        self_type& ways(uint32_t ways_)
        {
            m_ways = ways_;
            return *this;
        }

        self_type& pq_size(uint32_t pq_size_)
        {
            m_pq_size = pq_size_;
            return *this;
        }

        self_type& mshr_size(uint32_t mshr_size_)
        {
            m_mshr_size = mshr_size_;
            return *this;
        }

        self_type& latency(uint64_t lat_)
        {
            m_latency = lat_;
            return *this;
        }

        self_type& hit_latency(uint64_t hit_lat_)
        {
            m_hit_lat = hit_lat_;
            return *this;
        }

        self_type& fill_latency(uint64_t fill_lat_)
        {
            m_fill_lat = fill_lat_;
            return *this;
        }

        self_type& tag_bandwidth(uint32_t max_read_)
        {
            m_max_tag = max_read_;
            return *this;
        }

        self_type& fill_bandwidth(uint32_t max_write_)
        {
            m_max_fill = max_write_;
            return *this;
        }

        self_type& offset_bits(unsigned offset_bits_)
        {
            m_offset_bits = offset_bits_;
            return *this;
        }

        self_type& set_prefetch_as_load()
        {
            m_pref_load = true;
            return *this;
        }

        self_type& reset_prefetch_as_load()
        {
            m_pref_load = false;
            return *this;
        }

        self_type& set_wq_checks_full_addr()
        {
            m_wq_full_addr = true;
            return *this;
        }

        self_type& reset_wq_checks_full_addr()
        {
            m_wq_full_addr = false;
            return *this;
        }

        self_type& set_virtual_prefetch()
        {
            m_va_pref = true;
            return *this;
        }

        self_type& reset_virtual_prefetch()
        {
            m_va_pref = false;
            return *this;
        }

        template<typename... Elems>
        self_type& prefetch_activate(Elems... pref_act_elems)
        {
            m_pref_act_mask = ((1u << champsim::to_underlying(pref_act_elems)) | ... | 0);
            return *this;
        }

        self_type& upper_levels(std::vector<CACHE::channel_type*>&& uls_)
        {
            m_uls = std::move(uls_);
            return *this;
        }

        self_type& lower_level(CACHE::channel_type* ll_)
        {
            m_ll = ll_;
            return *this;
        }

        self_type& lower_translate(CACHE::channel_type* lt_)
        {
            m_lt = lt_;
            return *this;
        }

        template<unsigned long long P>
        Builder<P, R_FLAG> prefetcher()
        {
            return Builder<P, R_FLAG> {builder_conversion_tag {}, *this};
        }

        template<unsigned long long R>
        Builder<P_FLAG, R> replacement()
        {
            return Builder<P_FLAG, R> {builder_conversion_tag {}, *this};
        }
    };

    template<unsigned long long P_FLAG, unsigned long long R_FLAG>
    explicit CACHE(Builder<P_FLAG, R_FLAG> b)
    : champsim::operable(b.m_freq_scale), upper_levels(std::move(b.m_uls)), lower_level(b.m_ll), lower_translate(b.m_lt), NAME(b.m_name), NUM_SET(b.m_sets),
      NUM_WAY(b.m_ways), MSHR_SIZE(b.m_mshr_size), PQ_SIZE(b.m_pq_size), HIT_LATENCY((b.m_hit_lat > 0) ? b.m_hit_lat : b.m_latency - b.m_fill_lat),
      FILL_LATENCY(b.m_fill_lat), OFFSET_BITS(b.m_offset_bits), MAX_TAG(b.m_max_tag), MAX_FILL(b.m_max_fill), prefetch_as_load(b.m_pref_load),
      match_offset_bits(b.m_wq_full_addr), virtual_prefetch(b.m_va_pref), pref_activate_mask(b.m_pref_act_mask),
      module_pimpl(std::make_unique<module_model<P_FLAG, R_FLAG>>(this))
    {
    }
};

#if (USER_CODES == ENABLE)

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_initialize()
{
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) intern_->pref_prefetcherDip_stride_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) intern_->pref_prefetcherDnext_line_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) intern_->ipref_prefetcherDnext_line_instr_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) intern_->pref_prefetcherDno_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) intern_->ipref_prefetcherDno_instr_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) intern_->pref_prefetcherDspp_dev_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) intern_->pref_prefetcherDva_ampm_lite_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_initialize();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_initialize();
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
uint32_t CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, bool useful_prefetch, uint8_t type, uint32_t metadata_in)
{
    uint32_t result {};
    std::bit_xor<decltype(result)> joiner {};
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) result = joiner(result, intern_->pref_prefetcherDip_stride_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) result = joiner(result, intern_->pref_prefetcherDnext_line_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) result = joiner(result, intern_->ipref_prefetcherDnext_line_instr_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) result = joiner(result, intern_->pref_prefetcherDno_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) result = joiner(result, intern_->ipref_prefetcherDno_instr_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) result = joiner(result, intern_->pref_prefetcherDspp_dev_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) result = joiner(result, intern_->pref_prefetcherDva_ampm_lite_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cache_operate(addr, ip, cache_hit, useful_prefetch, type, metadata_in));
    return result;
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
uint32_t CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
    uint32_t result {};
    std::bit_xor<decltype(result)> joiner {};
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) result = joiner(result, intern_->pref_prefetcherDip_stride_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) result = joiner(result, intern_->pref_prefetcherDnext_line_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) result = joiner(result, intern_->ipref_prefetcherDnext_line_instr_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) result = joiner(result, intern_->pref_prefetcherDno_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) result = joiner(result, intern_->ipref_prefetcherDno_instr_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) result = joiner(result, intern_->pref_prefetcherDspp_dev_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) result = joiner(result, intern_->pref_prefetcherDva_ampm_lite_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) result = joiner(result, intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cache_fill(addr, set, way, prefetch, evicted_addr, metadata_in));
    return result;
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_cycle_operate()
{
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) intern_->pref_prefetcherDip_stride_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) intern_->pref_prefetcherDnext_line_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) intern_->ipref_prefetcherDnext_line_instr_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) intern_->pref_prefetcherDno_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) intern_->ipref_prefetcherDno_instr_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) intern_->pref_prefetcherDspp_dev_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) intern_->pref_prefetcherDva_ampm_lite_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_cycle_operate();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_cycle_operate();
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_final_stats()
{
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) intern_->pref_prefetcherDip_stride_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) intern_->pref_prefetcherDnext_line_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) intern_->ipref_prefetcherDnext_line_instr_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) intern_->pref_prefetcherDno_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) intern_->ipref_prefetcherDno_instr_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) intern_->pref_prefetcherDspp_dev_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) intern_->pref_prefetcherDva_ampm_lite_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_final_stats();
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_final_stats();
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_prefetcher_branch_operate(uint64_t ip, uint8_t branch_type, uint64_t branch_target)
{
    if constexpr ((P_FLAG & CACHE::pprefetcherDip_stride) != 0) intern_->pref_prefetcherDip_stride_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line) != 0) intern_->pref_prefetcherDnext_line_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDnext_line_instr) != 0) intern_->ipref_prefetcherDnext_line_instr_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDno) != 0) intern_->pref_prefetcherDno_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDno_instr) != 0) intern_->ipref_prefetcherDno_instr_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDspp_dev) != 0) intern_->pref_prefetcherDspp_dev_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::pprefetcherDva_ampm_lite) != 0) intern_->pref_prefetcherDva_ampm_lite_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDaddress_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDaddress_collector_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_collector_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDmetadata_emitter) != 0) intern_->pref_testDcppDmodulesDprefetcherDmetadata_emitter_prefetcher_branch_operate(ip, branch_type, branch_target);
    if constexpr ((P_FLAG & CACHE::ptestDcppDmodulesDprefetcherDprefetch_hit_collector) != 0) intern_->pref_testDcppDmodulesDprefetcherDprefetch_hit_collector_prefetcher_branch_operate(ip, branch_type, branch_target);
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_initialize_replacement()
{
    if constexpr ((R_FLAG & CACHE::rreplacementDdrrip) != 0) intern_->repl_replacementDdrrip_initialize_replacement();
    if constexpr ((R_FLAG & CACHE::rreplacementDlru) != 0) intern_->repl_replacementDlru_initialize_replacement();
    if constexpr ((R_FLAG & CACHE::rreplacementDship) != 0) intern_->repl_replacementDship_initialize_replacement();
    if constexpr ((R_FLAG & CACHE::rreplacementDsrrip) != 0) intern_->repl_replacementDsrrip_initialize_replacement();
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDlru_collect) != 0) intern_->repl_testDcppDmodulesDreplacementDlru_collect_initialize_replacement();
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDmock_replacement) != 0) intern_->repl_testDcppDmodulesDreplacementDmock_replacement_initialize_replacement();
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
uint32_t CACHE::module_model<P_FLAG, R_FLAG>::impl_find_victim(uint32_t triggering_cpu, uint64_t instr_id, uint32_t set, const BLOCK* current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    uint32_t result {};
    champsim::detail::take_last<decltype(result)> joiner {};
    if constexpr ((R_FLAG & CACHE::rreplacementDdrrip) != 0) result = joiner(result, intern_->repl_replacementDdrrip_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    if constexpr ((R_FLAG & CACHE::rreplacementDlru) != 0) result = joiner(result, intern_->repl_replacementDlru_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    if constexpr ((R_FLAG & CACHE::rreplacementDship) != 0) result = joiner(result, intern_->repl_replacementDship_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    if constexpr ((R_FLAG & CACHE::rreplacementDsrrip) != 0) result = joiner(result, intern_->repl_replacementDsrrip_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDlru_collect) != 0) result = joiner(result, intern_->repl_testDcppDmodulesDreplacementDlru_collect_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDmock_replacement) != 0) result = joiner(result, intern_->repl_testDcppDmodulesDreplacementDmock_replacement_find_victim(triggering_cpu, instr_id, set, current_set, ip, full_addr, type));
    return result;
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_update_replacement_state(uint32_t triggering_cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    if constexpr ((R_FLAG & CACHE::rreplacementDdrrip) != 0) intern_->repl_replacementDdrrip_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    if constexpr ((R_FLAG & CACHE::rreplacementDlru) != 0) intern_->repl_replacementDlru_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    if constexpr ((R_FLAG & CACHE::rreplacementDship) != 0) intern_->repl_replacementDship_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    if constexpr ((R_FLAG & CACHE::rreplacementDsrrip) != 0) intern_->repl_replacementDsrrip_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDlru_collect) != 0) intern_->repl_testDcppDmodulesDreplacementDlru_collect_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDmock_replacement) != 0) intern_->repl_testDcppDmodulesDreplacementDmock_replacement_update_replacement_state(triggering_cpu, set, way, full_addr, ip, victim_addr, type, hit);
}

template<unsigned long long P_FLAG, unsigned long long R_FLAG>
void CACHE::module_model<P_FLAG, R_FLAG>::impl_replacement_final_stats()
{
    if constexpr ((R_FLAG & CACHE::rreplacementDdrrip) != 0) intern_->repl_replacementDdrrip_replacement_final_stats();
    if constexpr ((R_FLAG & CACHE::rreplacementDlru) != 0) intern_->repl_replacementDlru_replacement_final_stats();
    if constexpr ((R_FLAG & CACHE::rreplacementDship) != 0) intern_->repl_replacementDship_replacement_final_stats();
    if constexpr ((R_FLAG & CACHE::rreplacementDsrrip) != 0) intern_->repl_replacementDsrrip_replacement_final_stats();
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDlru_collect) != 0) intern_->repl_testDcppDmodulesDreplacementDlru_collect_replacement_final_stats();
    if constexpr ((R_FLAG & CACHE::rtestDcppDmodulesDreplacementDmock_replacement) != 0) intern_->repl_testDcppDmodulesDreplacementDmock_replacement_replacement_final_stats();
}

#else
#include "cache_module_def.inc"
#endif // USER_CODES

#endif

#ifdef SET_ASIDE_CHAMPSIM_MODULE
#undef SET_ASIDE_CHAMPSIM_MODULE
#define CHAMPSIM_MODULE
#endif

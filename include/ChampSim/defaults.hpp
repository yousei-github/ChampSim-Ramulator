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

#ifndef DEFAULTS_HPP
#define DEFAULTS_HPP

#include "ChampSim/cache.h"
#include "ChampSim/champsim_constants.h"
#include "ChampSim/ooo_cpu.h"
#include "ChampSim/ptw.h"
#include "ProjectConfiguration.h" // User file

namespace champsim::defaults
{
const auto default_core = O3_CPU::Builder {}
                              .dib_set(CPU_DIB_SET)
                              .dib_way(CPU_DIB_WAY)
                              .dib_window(CPU_DIB_WINDOW)
                              .ifetch_buffer_size(CPU_IFETCH_BUFFER_SIZE)
                              .decode_buffer_size(CPU_DECODE_BUFFER_SIZE)
                              .dispatch_buffer_size(CPU_DISPATCH_BUFFER_SIZE)
                              .rob_size(CPU_ROB_SIZE)
                              .lq_size(CPU_LQ_SIZE)
                              .sq_size(CPU_SQ_SIZE)
                              .fetch_width(CPU_FETCH_WIDTH)
                              .decode_width(CPU_DECODE_WIDTH)
                              .dispatch_width(CPU_DISPATCH_WIDTH)
                              .execute_width(CPU_EXECUTE_WIDTH)
                              .lq_width(CPU_LQ_WIDTH)
                              .sq_width(CPU_SQ_WIDTH)
                              .retire_width(CPU_RETIRE_WIDTH)
                              .mispredict_penalty(CPU_MISPREDICT_PENALTY)
                              .schedule_width(CPU_SCHEDULER_SIZE)
                              .decode_latency(CPU_DECODE_LATENCY)
                              .dispatch_latency(CPU_DISPATCH_LATENCY)
                              .schedule_latency(CPU_SCHEDULE_LATENCY)
                              .execute_latency(CPU_EXECUTE_LATENCY)
                              .l1i_bandwidth(CPU_L1I_BANDWIDTH)
                              .l1d_bandwidth(CPU_L1D_BANDWIDTH)
                              // Specifying default branch predictors and BTBs like this is probably dangerous
                              // since the names could change.
                              // We're doing it anyway, for now.
                              .branch_predictor<BRANCH_USE_BIMODAL>()
                              .btb<BRANCH_TARGET_BUFFER>();

const auto default_l1i = CACHE::Builder {}
                             .sets(L1I_SETS)
                             .ways(L1I_WAYS)
                             .pq_size(L1I_PQ_SIZE)
                             .mshr_size(L1I_MSHR_SIZE)
                             .hit_latency(L1I_HIT_LATENCY)
                             .fill_latency(L1I_FILL_LATENCY)
                             .tag_bandwidth(L1I_TAG_BANDWIDTH)
                             .fill_bandwidth(L1I_FILL_BANDWIDTH)
                             .offset_bits(LOG2_BLOCK_SIZE)
                             .reset_prefetch_as_load()
                             .set_virtual_prefetch()
                             .set_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                             // Specifying default prefetchers and replacement policies like this is probably dangerous
                             // since the names could change.
                             // We're doing it anyway, for now.
                             .prefetcher<CPU_L1I_PREFETCHER>()
                             .replacement<CPU_L1I_REPLACEMENT_POLICY>();

const auto default_l1d = CACHE::Builder {}
                             .sets(L1D_SETS)
                             .ways(L1D_WAYS)
                             .pq_size(L1D_PQ_SIZE)
                             .mshr_size(L1D_MSHR_SIZE)
                             .hit_latency(L1D_HIT_LATENCY)
                             .fill_latency(L1D_FILL_LATENCY)
                             .tag_bandwidth(L1D_TAG_BANDWIDTH)
                             .fill_bandwidth(L1D_FILL_BANDWIDTH)
                             .offset_bits(LOG2_BLOCK_SIZE)
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .set_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                             .prefetcher<CPU_L1D_PREFETCHER>()
                             .replacement<CPU_L1D_REPLACEMENT_POLICY>();

const auto default_l2c = CACHE::Builder {}
                             .sets(L2C_SETS)
                             .ways(L2C_WAYS)
                             .pq_size(L2C_PQ_SIZE)
                             .mshr_size(L2C_MSHR_SIZE)
                             .hit_latency(L2C_HIT_LATENCY)
                             .fill_latency(L2C_FILL_LATENCY)
                             .tag_bandwidth(L2C_TAG_BANDWIDTH)
                             .fill_bandwidth(L2C_FILL_BANDWIDTH)
                             .offset_bits(LOG2_BLOCK_SIZE)
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .reset_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                             .prefetcher<CPU_L2C_PREFETCHER>()
                             .replacement<CPU_L2C_REPLACEMENT_POLICY>();

const auto default_itlb = CACHE::Builder {}
                              .sets(ITLB_SETS)
                              .ways(ITLB_WAYS)
                              .pq_size(ITLB_PQ_SIZE)
                              .mshr_size(ITLB_MSHR_SIZE)
                              .hit_latency(ITLB_HIT_LATENCY)
                              .fill_latency(ITLB_FILL_LATENCY)
                              .tag_bandwidth(ITLB_TAG_BANDWIDTH)
                              .fill_bandwidth(ITLB_FILL_BANDWIDTH)
                              .offset_bits(LOG2_PAGE_SIZE)
                              .reset_prefetch_as_load()
                              .set_virtual_prefetch()
                              .set_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                              .prefetcher<CPU_ITLB_PREFETCHER>()
                              .replacement<CPU_ITLB_REPLACEMENT_POLICY>();

const auto default_dtlb = CACHE::Builder {}
                              .sets(DTLB_SETS)
                              .ways(DTLB_WAYS)
                              .pq_size(DTLB_PQ_SIZE)
                              .mshr_size(DTLB_MSHR_SIZE)
                              .hit_latency(DTLB_HIT_LATENCY)
                              .fill_latency(DTLB_FILL_LATENCY)
                              .tag_bandwidth(DTLB_TAG_BANDWIDTH)
                              .fill_bandwidth(DTLB_FILL_BANDWIDTH)
                              .offset_bits(LOG2_PAGE_SIZE)
                              .reset_prefetch_as_load()
                              .reset_virtual_prefetch()
                              .set_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                              .prefetcher<CPU_DTLB_PREFETCHER>()
                              .replacement<CPU_DTLB_REPLACEMENT_POLICY>();

const auto default_stlb = CACHE::Builder {}
                              .sets(STLB_SETS)
                              .ways(STLB_WAYS)
                              .pq_size(STLB_PQ_SIZE)
                              .mshr_size(STLB_MSHR_SIZE)
                              .hit_latency(STLB_HIT_LATENCY)
                              .fill_latency(STLB_FILL_LATENCY)
                              .tag_bandwidth(STLB_TAG_BANDWIDTH)
                              .fill_bandwidth(STLB_FILL_BANDWIDTH)
                              .offset_bits(LOG2_PAGE_SIZE)
                              .reset_prefetch_as_load()
                              .reset_virtual_prefetch()
                              .reset_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                              .prefetcher<CPU_STLB_PREFETCHER>()
                              .replacement<CPU_STLB_REPLACEMENT_POLICY>();

const auto default_llc = CACHE::Builder {}
                             .name("LLC")
                             .sets(LLC_SETS)
                             .ways(LLC_WAYS)
                             .pq_size(LLC_PQ_SIZE)
                             .mshr_size(LLC_MSHR_SIZE)
                             .hit_latency(LLC_HIT_LATENCY)
                             .fill_latency(LLC_FILL_LATENCY)
                             .tag_bandwidth(LLC_TAG_BANDWIDTH)
                             .fill_bandwidth(LLC_FILL_BANDWIDTH)
                             .offset_bits(LOG2_BLOCK_SIZE)
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .reset_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH)
                             .prefetcher<LLC_PREFETCHER>()
                             .replacement<LLC_REPLACEMENT_POLICY>();

const auto default_ptw =
    PageTableWalker::Builder {}.tag_bandwidth(2).fill_bandwidth(2).mshr_size(PTW_MSHR_SIZE).add_pscl(5, 1, 2).add_pscl(4, 1, 4).add_pscl(3, 2, 4).add_pscl(2, 4, 8);

} // namespace champsim::defaults

#endif

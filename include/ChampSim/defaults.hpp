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

#include "ChampSim/branch/bimodal/bimodal.h"
#include "ChampSim/branch/hashed_perceptron/hashed_perceptron.h"
#include "ChampSim/btb/basic_btb/basic_btb.h"
#include "ChampSim/cache_builder.h"
#include "ChampSim/champsim_constants.h"
#include "ChampSim/core_builder.h"
#include "ChampSim/prefetcher/no/no.h"
#include "ChampSim/ptw_builder.h"
#include "ChampSim/replacement/lru/lru.h"
#include "ProjectConfiguration.h" // User file

namespace champsim::defaults
{
const auto default_core =
    champsim::core_builder<champsim::core_builder_module_type_holder<CPU_BRANCH_PREDICTOR>, champsim::core_builder_module_type_holder<CPU_BRANCH_TARGET_BUFFER>> {}
        .dib_set(CPU_DIB_SET)
        .dib_way(CPU_DIB_WAY)
        .dib_window(CPU_DIB_WINDOW)
        .ifetch_buffer_size(CPU_IFETCH_BUFFER_SIZE)
        .decode_buffer_size(CPU_DECODE_BUFFER_SIZE)
        .dispatch_buffer_size(CPU_DISPATCH_BUFFER_SIZE)
        .dib_hit_buffer_size(CPU_DIB_HIT_BUFFER_SIZE) // assumed
        .register_file_size(CPU_REGISTER_FILE_SIZE)
        .rob_size(CPU_ROB_SIZE)
        .lq_size(CPU_LQ_SIZE)
        .sq_size(CPU_SQ_SIZE)
        .fetch_width(champsim::bandwidth::maximum_type {CPU_FETCH_WIDTH})
        .decode_width(champsim::bandwidth::maximum_type {CPU_DECODE_WIDTH})
        .dispatch_width(champsim::bandwidth::maximum_type {CPU_DISPATCH_WIDTH})
        .execute_width(champsim::bandwidth::maximum_type {CPU_EXECUTE_WIDTH})
        .lq_width(champsim::bandwidth::maximum_type {CPU_LQ_WIDTH})
        .sq_width(champsim::bandwidth::maximum_type {CPU_SQ_WIDTH})
        .retire_width(champsim::bandwidth::maximum_type {CPU_RETIRE_WIDTH})
        .dib_inorder_width(champsim::bandwidth::maximum_type {CPU_DIB_INORDER_WIDTH}) // assumed
        .mispredict_penalty(CPU_MISPREDICT_PENALTY)
        .schedule_width(champsim::bandwidth::maximum_type {CPU_SCHEDULER_SIZE})
        .decode_latency(CPU_DECODE_LATENCY)
        .dib_hit_latency(CPU_DIB_HIT_LATENCY)
        .dispatch_latency(CPU_DISPATCH_LATENCY)
        .schedule_latency(CPU_SCHEDULE_LATENCY)
        .execute_latency(CPU_EXECUTE_LATENCY)
        .l1i_bandwidth(champsim::bandwidth::maximum_type {CPU_L1I_BANDWIDTH})
        .l1d_bandwidth(champsim::bandwidth::maximum_type {CPU_L1D_BANDWIDTH});

const auto default_l1i = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_L1I_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_L1I_REPLACEMENT_POLICY>> {}
                             .sets_factor(L1I_SETS)
                             .ways(L1I_WAYS)
                             .pq_size(L1I_PQ_SIZE)
                             .offset_bits(champsim::data::bits {LOG2_BLOCK_SIZE})
                             .reset_prefetch_as_load()
                             .set_virtual_prefetch()
                             .set_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_l1d = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_L1D_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_L1D_REPLACEMENT_POLICY>> {}
                             .sets_factor(L1D_SETS)
                             .ways(L1D_WAYS)
                             .pq_size(L1D_PQ_SIZE)
                             .offset_bits(champsim::data::bits {LOG2_BLOCK_SIZE})
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .set_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_l2c = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_L2C_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_L2C_REPLACEMENT_POLICY>> {}
                             .sets_factor(L2C_SETS)
                             .ways(L2C_WAYS)
                             .pq_size(L2C_PQ_SIZE)
                             .offset_bits(champsim::data::bits {LOG2_BLOCK_SIZE})
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .reset_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_itlb = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_ITLB_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_ITLB_REPLACEMENT_POLICY>> {}
                              .sets_factor(ITLB_SETS)
                              .ways(ITLB_WAYS)
                              .pq_size(ITLB_PQ_SIZE)
                              .offset_bits(champsim::data::bits {LOG2_PAGE_SIZE})
                              .reset_prefetch_as_load()
                              .set_virtual_prefetch()
                              .set_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_dtlb = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_DTLB_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_DTLB_REPLACEMENT_POLICY>> {}
                              .sets_factor(DTLB_SETS)
                              .ways(DTLB_WAYS)
                              .pq_size(DTLB_PQ_SIZE)
                              .mshr_size(DTLB_MSHR_SIZE)
                              .offset_bits(champsim::data::bits {LOG2_PAGE_SIZE})
                              .reset_prefetch_as_load()
                              .reset_virtual_prefetch()
                              .set_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_stlb = champsim::cache_builder<champsim::cache_builder_module_type_holder<CPU_STLB_PREFETCHER>, champsim::cache_builder_module_type_holder<CPU_STLB_REPLACEMENT_POLICY>> {}
                              .sets_factor(STLB_SETS)
                              .ways(STLB_WAYS)
                              .pq_size(STLB_PQ_SIZE)
                              .offset_bits(champsim::data::bits {LOG2_PAGE_SIZE})
                              .reset_prefetch_as_load()
                              .reset_virtual_prefetch()
                              .reset_wq_checks_full_addr()
                              .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_llc = champsim::cache_builder<champsim::cache_builder_module_type_holder<LLC_PREFETCHER>, champsim::cache_builder_module_type_holder<LLC_REPLACEMENT_POLICY>> {}
                             .name("LLC")
                             .sets_factor(LLC_SETS)
                             .ways(LLC_WAYS)
                             .pq_size(LLC_PQ_SIZE)
                             .offset_bits(champsim::data::bits {LOG2_BLOCK_SIZE})
                             .reset_prefetch_as_load()
                             .reset_virtual_prefetch()
                             .reset_wq_checks_full_addr()
                             .prefetch_activate(access_type::LOAD, access_type::PREFETCH);

const auto default_ptw = champsim::ptw_builder {}.bandwidth_factor(2).mshr_factor(PTW_MSHR_SIZE).add_pscl(5, 1, 2).add_pscl(4, 1, 4).add_pscl(3, 2, 4).add_pscl(2, 4, 8);
} // namespace champsim::defaults

#endif

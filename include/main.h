#ifndef __MAIN_H
#define __MAIN_H

/* Header */
// Includes for ChampSim
#include <fmt/core.h>

#include <CLI/CLI.hpp>
#include <algorithm>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

#include "ChampSim/champsim.h"
#include "ChampSim/champsim_constants.h"
#include "ChampSim/defaults.hpp"
#include "ChampSim/dram_controller.h"
#include "ChampSim/environment.h"
#include "ChampSim/phase_info.h"
#include "ChampSim/stats_printer.h"
#include "ChampSim/tracereader.h"
#include "ChampSim/vmem.h"

// Includes for Ramulator
#include <stdlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>

#include "Ramulator/Config.h"
#include "Ramulator/Controller.h"
#include "Ramulator/DRAM.h"
#include "Ramulator/Memory.h"
#include "Ramulator/Processor.h"
#include "Ramulator/SpeedyController.h"
#include "Ramulator/Statistics.h"

// Standards
#include "Ramulator/ALDRAM.h"
#include "Ramulator/DDR3.h"
#include "Ramulator/DDR4.h"
#include "Ramulator/DSARP.h"
#include "Ramulator/GDDR5.h"
#include "Ramulator/Gem5Wrapper.h"
#include "Ramulator/HBM.h"
#include "Ramulator/LPDDR3.h"
#include "Ramulator/LPDDR4.h"
#include "Ramulator/PCM.h"
#include "Ramulator/SALP.h"
#include "Ramulator/STTMRAM.h"
#include "Ramulator/TLDRAM.h"
#include "Ramulator/WideIO.h"
#include "Ramulator/WideIO2.h"

// Includes for this project
#include "ProjectConfiguration.h" // User file

/* Macro */

/* Type */

/* Prototype */
#if (USER_CODES == ENABLE)

#if (RAMULATOR == ENABLE)

#if (MEMORY_USE_HYBRID == ENABLE)

namespace champsim::configured
{
template<typename MEMORY_TYPE, typename MEMORY_TYPE2>
class generated_environment final : public champsim::environment
{
public:
    champsim::channel LLC_to_MEMORY_CONTROLLER_queues {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_STLB_to_cpu0_PTW_queues {32, 0, 0, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_DTLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_ITLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L2C_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L1D_to_cpu0_L2C_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_L1I_to_cpu0_L2C_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_to_cpu0_L1I_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_PTW_to_cpu0_L1D_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_to_cpu0_L1D_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_L1I_to_cpu0_ITLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L1D_to_cpu0_DTLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L2C_to_LLC_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};

    // Memory controller
    MEMORY_CONTROLLER<MEMORY_TYPE, MEMORY_TYPE2> memory_controller;

    // Virtual memory
    VirtualMemory vmem;

    PageTableWalker cpu0_PTW {PageTableWalker::Builder {champsim::defaults::default_ptw}
                                  .name("cpu0_PTW")
                                  .cpu(PTW_CPU_ID)
                                  .virtual_memory(&vmem)
                                  .upper_levels({&cpu0_STLB_to_cpu0_PTW_queues})
                                  .lower_level(&cpu0_PTW_to_cpu0_L1D_queues)};

    CACHE LLC {CACHE::Builder {champsim::defaults::default_llc}
                   .name("LLC")
                   .frequency(CACHE_CLOCK_SCALE)
                   .upper_levels({&cpu0_L2C_to_LLC_queues})
                   .lower_level(&LLC_to_MEMORY_CONTROLLER_queues)};

    CACHE cpu0_DTLB {CACHE::Builder {champsim::defaults::default_dtlb}
                         .name("cpu0_DTLB")
                         .frequency(CACHE_CLOCK_SCALE)
                         .upper_levels({&cpu0_L1D_to_cpu0_DTLB_queues})
                         .lower_level(&cpu0_DTLB_to_cpu0_STLB_queues)};

    CACHE cpu0_ITLB {CACHE::Builder {champsim::defaults::default_itlb}
                         .name("cpu0_ITLB")
                         .frequency(CACHE_CLOCK_SCALE)
                         .upper_levels({&cpu0_L1I_to_cpu0_ITLB_queues})
                         .lower_level(&cpu0_ITLB_to_cpu0_STLB_queues)};

    CACHE cpu0_L1D {
        CACHE::Builder {                         champsim::defaults::default_l1d}
            .name("cpu0_L1D")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_PTW_to_cpu0_L1D_queues, &cpu0_to_cpu0_L1D_queues}}
         )
            .lower_level(&cpu0_L1D_to_cpu0_L2C_queues)
            .lower_translate(&cpu0_L1D_to_cpu0_DTLB_queues)
    };

    CACHE cpu0_L1I {CACHE::Builder {champsim::defaults::default_l1i}
                        .name("cpu0_L1I")
                        .frequency(CACHE_CLOCK_SCALE)
                        .upper_levels({&cpu0_to_cpu0_L1I_queues})
                        .lower_level(&cpu0_L1I_to_cpu0_L2C_queues)
                        .lower_translate(&cpu0_L1I_to_cpu0_ITLB_queues)};

    CACHE cpu0_L2C {
        CACHE::Builder {                             champsim::defaults::default_l2c}
            .name("cpu0_L2C")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_L1D_to_cpu0_L2C_queues, &cpu0_L1I_to_cpu0_L2C_queues}}
         )
            .lower_level(&cpu0_L2C_to_LLC_queues)
            .lower_translate(&cpu0_L2C_to_cpu0_STLB_queues)
    };

    CACHE cpu0_STLB {
        CACHE::Builder {                                                               champsim::defaults::default_stlb}
            .name("cpu0_STLB")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_DTLB_to_cpu0_STLB_queues, &cpu0_ITLB_to_cpu0_STLB_queues, &cpu0_L2C_to_cpu0_STLB_queues}}
         )
            .lower_level(&cpu0_STLB_to_cpu0_PTW_queues)
    };

    // CPU 0
    O3_CPU cpu0 {O3_CPU::Builder {champsim::defaults::default_core}
                     .index(CPU_0)
                     .frequency(O3_CPU_CLOCK_SCALE)
                     .l1i(&cpu0_L1I)
                     .l1i_bandwidth(cpu0_L1I.MAX_TAG)
                     .l1d_bandwidth(cpu0_L1D.MAX_TAG)
                     .branch_predictor<BRANCH_PREDICTOR>()
                     .btb<BRANCH_TARGET_BUFFER>()
                     .fetch_queues(&cpu0_to_cpu0_L1I_queues)
                     .data_queues(&cpu0_to_cpu0_L1D_queues)};

    generated_environment(ramulator::Memory<MEMORY_TYPE, ramulator::Controller>& memory, ramulator::Memory<MEMORY_TYPE2, ramulator::Controller>& memory2)
    : memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, CPU_FREQUENCY / memory2.spec->speed_entry.freq, {&LLC_to_MEMORY_CONTROLLER_queues}, memory, memory2),
      vmem(4096, PAGE_TABLE_LEVELS, MINOR_FAULT_PENALTY, memory.max_address + memory2.max_address)
    {
    }

    ~generated_environment()
    {
    }

    std::vector<std::reference_wrapper<O3_CPU>> cpu_view() override
    {
        return {std::ref(cpu0)};
    }

    std::vector<std::reference_wrapper<CACHE>> cache_view() override
    {
        return {LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB};
    }

    std::vector<std::reference_wrapper<PageTableWalker>> ptw_view() override
    {
        return {cpu0_PTW};
    }

    std::vector<std::reference_wrapper<champsim::operable>> operable_view() override
    {
        return {cpu0, cpu0_PTW, LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB, memory_controller};
    }
};
} // namespace champsim::configured

#else

namespace champsim::configured
{
template<typename MEMORY_TYPE>
class generated_environment final : public champsim::environment
{
public:
    champsim::channel LLC_to_MEMORY_CONTROLLER_queues {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_STLB_to_cpu0_PTW_queues {32, 0, 0, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_DTLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_ITLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L2C_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L1D_to_cpu0_L2C_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_L1I_to_cpu0_L2C_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};
    champsim::channel cpu0_to_cpu0_L1I_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_PTW_to_cpu0_L1D_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_to_cpu0_L1D_queues {32, 32, 32, LOG2_BLOCK_SIZE, 1};
    champsim::channel cpu0_L1I_to_cpu0_ITLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L1D_to_cpu0_DTLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L2C_to_LLC_queues {32, 32, 32, LOG2_BLOCK_SIZE, 0};

    // Memory controller
    MEMORY_CONTROLLER<MEMORY_TYPE> memory_controller;

    // Virtual memory
    VirtualMemory vmem;

    PageTableWalker cpu0_PTW {PageTableWalker::Builder {champsim::defaults::default_ptw}
                                  .name("cpu0_PTW")
                                  .cpu(PTW_CPU_ID)
                                  .virtual_memory(&vmem)
                                  .upper_levels({&cpu0_STLB_to_cpu0_PTW_queues})
                                  .lower_level(&cpu0_PTW_to_cpu0_L1D_queues)};

    CACHE LLC {CACHE::Builder {champsim::defaults::default_llc}
                   .name("LLC")
                   .frequency(CACHE_CLOCK_SCALE)
                   .upper_levels({&cpu0_L2C_to_LLC_queues})
                   .lower_level(&LLC_to_MEMORY_CONTROLLER_queues)};

    CACHE cpu0_DTLB {CACHE::Builder {champsim::defaults::default_dtlb}
                         .name("cpu0_DTLB")
                         .frequency(CACHE_CLOCK_SCALE)
                         .upper_levels({&cpu0_L1D_to_cpu0_DTLB_queues})
                         .lower_level(&cpu0_DTLB_to_cpu0_STLB_queues)};

    CACHE cpu0_ITLB {CACHE::Builder {champsim::defaults::default_itlb}
                         .name("cpu0_ITLB")
                         .frequency(CACHE_CLOCK_SCALE)
                         .upper_levels({&cpu0_L1I_to_cpu0_ITLB_queues})
                         .lower_level(&cpu0_ITLB_to_cpu0_STLB_queues)};

    CACHE cpu0_L1D {
        CACHE::Builder {                         champsim::defaults::default_l1d}
            .name("cpu0_L1D")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_PTW_to_cpu0_L1D_queues, &cpu0_to_cpu0_L1D_queues}}
         )
            .lower_level(&cpu0_L1D_to_cpu0_L2C_queues)
            .lower_translate(&cpu0_L1D_to_cpu0_DTLB_queues)
    };

    CACHE cpu0_L1I {CACHE::Builder {champsim::defaults::default_l1i}
                        .name("cpu0_L1I")
                        .frequency(CACHE_CLOCK_SCALE)
                        .upper_levels({&cpu0_to_cpu0_L1I_queues})
                        .lower_level(&cpu0_L1I_to_cpu0_L2C_queues)
                        .lower_translate(&cpu0_L1I_to_cpu0_ITLB_queues)};

    CACHE cpu0_L2C {
        CACHE::Builder {                             champsim::defaults::default_l2c}
            .name("cpu0_L2C")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_L1D_to_cpu0_L2C_queues, &cpu0_L1I_to_cpu0_L2C_queues}}
         )
            .lower_level(&cpu0_L2C_to_LLC_queues)
            .lower_translate(&cpu0_L2C_to_cpu0_STLB_queues)
    };

    CACHE cpu0_STLB {
        CACHE::Builder {                                                               champsim::defaults::default_stlb}
            .name("cpu0_STLB")
            .frequency(CACHE_CLOCK_SCALE)
            .upper_levels({{&cpu0_DTLB_to_cpu0_STLB_queues, &cpu0_ITLB_to_cpu0_STLB_queues, &cpu0_L2C_to_cpu0_STLB_queues}}
         )
            .lower_level(&cpu0_STLB_to_cpu0_PTW_queues)
    };

    // CPU 0
    O3_CPU cpu0 {O3_CPU::Builder {champsim::defaults::default_core}
                     .index(CPU_0)
                     .frequency(O3_CPU_CLOCK_SCALE)
                     .l1i(&cpu0_L1I)
                     .l1i_bandwidth(cpu0_L1I.MAX_TAG)
                     .l1d_bandwidth(cpu0_L1D.MAX_TAG)
                     .branch_predictor<BRANCH_PREDICTOR>()
                     .btb<BRANCH_TARGET_BUFFER>()
                     .fetch_queues(&cpu0_to_cpu0_L1I_queues)
                     .data_queues(&cpu0_to_cpu0_L1D_queues)};

    generated_environment(ramulator::Memory<MEMORY_TYPE, ramulator::Controller>& memory)
    : memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, {&LLC_to_MEMORY_CONTROLLER_queues}, memory),
      vmem(4096, PAGE_TABLE_LEVELS, MINOR_FAULT_PENALTY, memory.max_address)

    {
    }

    ~generated_environment()
    {
    }

    std::vector<std::reference_wrapper<O3_CPU>> cpu_view() override
    {
        return {std::ref(cpu0)};
    }

    std::vector<std::reference_wrapper<CACHE>> cache_view() override
    {
        return {LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB};
    }

    std::vector<std::reference_wrapper<PageTableWalker>> ptw_view() override
    {
        return {cpu0_PTW};
    }

    std::vector<std::reference_wrapper<champsim::operable>> operable_view() override
    {
        return {cpu0, cpu0_PTW, LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB, memory_controller};
    }
};
} // namespace champsim::configured

#endif // MEMORY_USE_HYBRID

#else
/* Original code of ChampSim */

namespace champsim::configured
{
struct generated_environment final : public champsim::environment
{
    champsim::channel LLC_to_DRAM_queues {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), champsim::lg2(BLOCK_SIZE), 0};
    champsim::channel cpu0_STLB_to_cpu0_PTW_queues {32, 0, 0, champsim::lg2(PAGE_SIZE), 0};
    champsim::channel cpu0_DTLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_ITLB_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L2C_to_cpu0_STLB_queues {32, 32, 32, LOG2_PAGE_SIZE, 0};
    champsim::channel cpu0_L1D_to_cpu0_L2C_queues {32, 32, 32, champsim::lg2(64), 0};
    champsim::channel cpu0_L1I_to_cpu0_L2C_queues {32, 32, 32, champsim::lg2(64), 0};
    champsim::channel cpu0_to_cpu0_L1I_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_PTW_to_cpu0_L1D_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_to_cpu0_L1D_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_L1I_to_cpu0_ITLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L1D_to_cpu0_DTLB_queues {16, 16, 16, LOG2_PAGE_SIZE, 1};
    champsim::channel cpu0_L2C_to_LLC_queues {32, 32, 32, champsim::lg2(64), 0};

    MEMORY_CONTROLLER DRAM {1.25, 3200, 12.5, 12.5, 12.5, 7.5, {&LLC_to_DRAM_queues}};
    VirtualMemory vmem {4096, 5, 200, DRAM};
    PageTableWalker cpu0_PTW {PageTableWalker::Builder {champsim::defaults::default_ptw}
                                  .name("cpu0_PTW")
                                  .cpu(0)
                                  .virtual_memory(&vmem)
                                  .mshr_size(5)
                                  .upper_levels({&cpu0_STLB_to_cpu0_PTW_queues})
                                  .lower_level(&cpu0_PTW_to_cpu0_L1D_queues)};

    CACHE LLC {CACHE::Builder {champsim::defaults::default_llc}
                   .name("LLC")
                   .frequency(1.0)
                   .sets(2048)
                   .pq_size(32)
                   .mshr_size(64)
                   .tag_bandwidth(1)
                   .fill_bandwidth(1)
                   .offset_bits(champsim::lg2(64))
                   .replacement<CACHE::rreplacementDlru>()
                   .prefetcher<CACHE::pprefetcherDno>()
                   .upper_levels({&cpu0_L2C_to_LLC_queues})
                   .lower_level(&LLC_to_DRAM_queues)};

    CACHE cpu0_DTLB {CACHE::Builder {champsim::defaults::default_dtlb}
                         .name("cpu0_DTLB")
                         .frequency(1.0)
                         .sets(16)
                         .pq_size(16)
                         .mshr_size(8)
                         .tag_bandwidth(1)
                         .fill_bandwidth(1)
                         .offset_bits(LOG2_PAGE_SIZE)
                         .replacement<CACHE::rreplacementDlru>()
                         .prefetcher<CACHE::pprefetcherDno>()
                         .upper_levels({&cpu0_L1D_to_cpu0_DTLB_queues})
                         .lower_level(&cpu0_DTLB_to_cpu0_STLB_queues)};

    CACHE cpu0_ITLB {CACHE::Builder {champsim::defaults::default_itlb}
                         .name("cpu0_ITLB")
                         .frequency(1.0)
                         .sets(16)
                         .pq_size(16)
                         .mshr_size(8)
                         .tag_bandwidth(1)
                         .fill_bandwidth(1)
                         .offset_bits(LOG2_PAGE_SIZE)
                         .replacement<CACHE::rreplacementDlru>()
                         .prefetcher<CACHE::pprefetcherDno>()
                         .upper_levels({&cpu0_L1I_to_cpu0_ITLB_queues})
                         .lower_level(&cpu0_ITLB_to_cpu0_STLB_queues)};

    CACHE cpu0_L1D {
        CACHE::Builder {                         champsim::defaults::default_l1d}
            .name("cpu0_L1D")
            .frequency(1.0)
            .sets(64)
            .pq_size(32)
            .mshr_size(32)
            .tag_bandwidth(1)
            .fill_bandwidth(1)
            .offset_bits(champsim::lg2(64))
            .replacement<CACHE::rreplacementDlru>()
            .prefetcher<CACHE::pprefetcherDno>()
            .upper_levels({{&cpu0_PTW_to_cpu0_L1D_queues, &cpu0_to_cpu0_L1D_queues}}
         )
            .lower_level(&cpu0_L1D_to_cpu0_L2C_queues)
            .lower_translate(&cpu0_L1D_to_cpu0_DTLB_queues)
    };

    CACHE cpu0_L1I {CACHE::Builder {champsim::defaults::default_l1i}
                        .name("cpu0_L1I")
                        .frequency(1.0)
                        .sets(64)
                        .pq_size(32)
                        .mshr_size(32)
                        .tag_bandwidth(1)
                        .fill_bandwidth(1)
                        .offset_bits(champsim::lg2(64))
                        .replacement<CACHE::rreplacementDlru>()
                        .prefetcher<CACHE::pprefetcherDno_instr>()
                        .upper_levels({&cpu0_to_cpu0_L1I_queues})
                        .lower_level(&cpu0_L1I_to_cpu0_L2C_queues)
                        .lower_translate(&cpu0_L1I_to_cpu0_ITLB_queues)};

    CACHE cpu0_L2C {
        CACHE::Builder {                             champsim::defaults::default_l2c}
            .name("cpu0_L2C")
            .frequency(1.0)
            .sets(1024)
            .pq_size(32)
            .mshr_size(64)
            .tag_bandwidth(1)
            .fill_bandwidth(1)
            .offset_bits(champsim::lg2(64))
            .replacement<CACHE::rreplacementDlru>()
            .prefetcher<CACHE::pprefetcherDno>()
            .upper_levels({{&cpu0_L1D_to_cpu0_L2C_queues, &cpu0_L1I_to_cpu0_L2C_queues}}
         )
            .lower_level(&cpu0_L2C_to_LLC_queues)
            .lower_translate(&cpu0_L2C_to_cpu0_STLB_queues)
    };

    CACHE cpu0_STLB {
        CACHE::Builder {                                                               champsim::defaults::default_stlb}
            .name("cpu0_STLB")
            .frequency(1.0)
            .sets(128)
            .pq_size(32)
            .mshr_size(16)
            .tag_bandwidth(1)
            .fill_bandwidth(1)
            .offset_bits(LOG2_PAGE_SIZE)
            .replacement<CACHE::rreplacementDlru>()
            .prefetcher<CACHE::pprefetcherDno>()
            .upper_levels({{&cpu0_DTLB_to_cpu0_STLB_queues, &cpu0_ITLB_to_cpu0_STLB_queues, &cpu0_L2C_to_cpu0_STLB_queues}}
         )
            .lower_level(&cpu0_STLB_to_cpu0_PTW_queues)
    };

    O3_CPU cpu0 {O3_CPU::Builder {champsim::defaults::default_core}
                     .index(0)
                     .frequency(1.0)
                     .l1i(&cpu0_L1I)
                     .l1i_bandwidth(cpu0_L1I.MAX_TAG)
                     .l1d_bandwidth(cpu0_L1D.MAX_TAG)
                     .branch_predictor<O3_CPU::bbranchDhashed_perceptron>()
                     .btb<O3_CPU::tbtbDbasic_btb>()
                     .fetch_queues(&cpu0_to_cpu0_L1I_queues)
                     .data_queues(&cpu0_to_cpu0_L1D_queues)};

    std::vector<std::reference_wrapper<O3_CPU>> cpu_view() override
    {
        return {std::ref(cpu0)};
    }

    std::vector<std::reference_wrapper<CACHE>> cache_view() override
    {
        return {LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB};
    }

    std::vector<std::reference_wrapper<PageTableWalker>> ptw_view() override
    {
        return {cpu0_PTW};
    }

    MEMORY_CONTROLLER& dram_view() override { return DRAM; }

    std::vector<std::reference_wrapper<champsim::operable>> operable_view() override
    {
        return {cpu0, cpu0_PTW, LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB, DRAM};
    }
};
} // namespace champsim::configured
#endif // RAMULATOR

/* Variable */

/* Function */

#endif // USER_CODES

#endif

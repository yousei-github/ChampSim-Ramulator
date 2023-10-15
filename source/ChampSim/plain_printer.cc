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

#include "ProjectConfiguration.h" // User file

#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>
#include <fmt/ostream.h>
#endif // USE_VCPKG

#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "ChampSim/stats_printer.h"

#if (USER_CODES == ENABLE)

void champsim::plain_printer::print(O3_CPU::stats_type stats)
{
    constexpr std::array<std::pair<std::string_view, std::size_t>, 6> types {
        {std::pair {"BRANCH_DIRECT_JUMP", BRANCH_DIRECT_JUMP}, std::pair {"BRANCH_INDIRECT", BRANCH_INDIRECT}, std::pair {"BRANCH_CONDITIONAL", BRANCH_CONDITIONAL},
         std::pair {"BRANCH_DIRECT_CALL", BRANCH_DIRECT_CALL}, std::pair {"BRANCH_INDIRECT_CALL", BRANCH_INDIRECT_CALL},
         std::pair {"BRANCH_RETURN", BRANCH_RETURN}}
    };

    auto total_branch         = std::ceil(std::accumulate(std::begin(types), std::end(types), 0ll, [tbt = stats.total_branch_types](auto acc, auto next)
                { return acc + tbt[next.second]; }));
    auto total_mispredictions = std::ceil(std::accumulate(std::begin(types), std::end(types), 0ll, [btm = stats.branch_type_misses](auto acc, auto next)
        { return acc + btm[next.second]; }));

#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "\n{} cumulative IPC: {:.4g} instructions: {} cycles: {}\n", stats.name, std::ceil(stats.instrs()) / std::ceil(stats.cycles()), stats.instrs(), stats.cycles());
    fmt::print(stream, "{} Branch Prediction Accuracy: {:.4g}% MPKI: {:.4g} Average ROB Occupancy at Mispredict: {:.4g}\n", stats.name, (100.0 * std::ceil(total_branch - total_mispredictions)) / total_branch, (1000.0 * total_mispredictions) / std::ceil(stats.instrs()), std::ceil(stats.total_rob_occupancy_at_branch_mispredict) / total_mispredictions);
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n%s cumulative IPC: %.4f instructions: %ld cycles: %ld\n", stats.name.c_str(), std::ceil(stats.instrs()) / std::ceil(stats.cycles()), stats.instrs(), stats.cycles());
    std::fprintf(output_statistics.file_handler, "%s Branch Prediction Accuracy: %.4f%% MPKI: %.4f Average ROB Occupancy at Mispredict: %.4f\n", stats.name.c_str(), (100.0 * std::ceil(total_branch - total_mispredictions)) / total_branch, (1000.0 * total_mispredictions) / std::ceil(stats.instrs()), std::ceil(stats.total_rob_occupancy_at_branch_mispredict) / total_mispredictions);
#endif // PRINT_STATISTICS_INTO_FILE

    std::vector<double> mpkis;
    std::transform(std::begin(stats.branch_type_misses), std::end(stats.branch_type_misses), std::back_inserter(mpkis),
        [instrs = stats.instrs()](auto x)
        { return 1000.0 * std::ceil(x) / std::ceil(instrs); });

#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "Branch type MPKI\n");
    for (auto [str, idx] : types)
    {
        fmt::print(stream, "{}: {:.3}\n", str, mpkis[idx]);
    }
    fmt::print(stream, "\n");

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "Branch type MPKI\n");
    for (auto [str, idx] : types)
    {
        std::fprintf(output_statistics.file_handler, "%s: %.3f\n", std::string(str).c_str(), mpkis[idx]);
    }
    std::fprintf(output_statistics.file_handler, "\n");

#endif // PRINT_STATISTICS_INTO_FILE
}

void champsim::plain_printer::print(CACHE::stats_type stats)
{
    constexpr std::array<std::pair<std::string_view, std::size_t>, 5> types {
        {std::pair {"LOAD", champsim::to_underlying(access_type::LOAD)}, std::pair {"RFO", champsim::to_underlying(access_type::RFO)},
         std::pair {"PREFETCH", champsim::to_underlying(access_type::PREFETCH)}, std::pair {"WRITE", champsim::to_underlying(access_type::WRITE)},
         std::pair {"TRANSLATION", champsim::to_underlying(access_type::TRANSLATION)}}
    };

    for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu)
    {
        uint64_t TOTAL_HIT = 0, TOTAL_MISS = 0;
        for (const auto& type : types)
        {
            TOTAL_HIT += stats.hits.at(type.second).at(cpu);
            TOTAL_MISS += stats.misses.at(type.second).at(cpu);
        }

#if (USE_VCPKG == ENABLE)
        fmt::print(stream, "{} TOTAL        ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, TOTAL_HIT + TOTAL_MISS, TOTAL_HIT, TOTAL_MISS);
        for (const auto& type : types)
        {
            fmt::print(stream, "{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, type.first, stats.hits[type.second][cpu] + stats.misses[type.second][cpu], stats.hits[type.second][cpu], stats.misses[type.second][cpu]);
        }
        fmt::print(stream, "{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}\n", stats.name, stats.pf_requested, stats.pf_issued, stats.pf_useful, stats.pf_useless);
        fmt::print(stream, "{} AVERAGE MISS LATENCY: {:.4g} cycles\n", stats.name, stats.avg_miss_latency);

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "%s TOTAL        ACCESS: %10ld HIT: %10ld MISS: %10ld\n", stats.name.c_str(), TOTAL_HIT + TOTAL_MISS, TOTAL_HIT, TOTAL_MISS);
        for (const auto& type : types)
        {
            std::fprintf(output_statistics.file_handler, "%s %12s ACCESS: %10ld HIT: %10ld MISS: %10ld\n", stats.name.c_str(), std::string(type.first).c_str(), stats.hits[type.second][cpu] + stats.misses[type.second][cpu], stats.hits[type.second][cpu], stats.misses[type.second][cpu]);
        }
        std::fprintf(output_statistics.file_handler, "%s PREFETCH REQUESTED: %10ld ISSUED: %10ld USEFUL: %10ld USELESS: %10ld\n", stats.name.c_str(), stats.pf_requested, stats.pf_issued, stats.pf_useful, stats.pf_useless);
        std::fprintf(output_statistics.file_handler, "%s AVERAGE MISS LATENCY: %.4lf cycles\n", stats.name.c_str(), stats.avg_miss_latency);
#endif // PRINT_STATISTICS_INTO_FILE
    }
}

void champsim::plain_printer::print(DRAM_CHANNEL::stats_type stats)
{
#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "\n{} RQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n", stats.name, stats.RQ_ROW_BUFFER_HIT, stats.RQ_ROW_BUFFER_MISS);
    if (stats.dbus_count_congested > 0)
    {
        fmt::print(stream, " AVG DBUS CONGESTED CYCLE: {:.4g}\n", std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested));
    }
    else
    {
        fmt::print(stream, " AVG DBUS CONGESTED CYCLE: -\n");
    }
    fmt::print(stream, "WQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n  FULL: {:10}\n", stats.name, stats.WQ_ROW_BUFFER_HIT, stats.WQ_ROW_BUFFER_MISS, stats.WQ_FULL);

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n%s RQ ROW_BUFFER_HIT: %10d\n  ROW_BUFFER_MISS: %10d\n", stats.name.c_str(), stats.RQ_ROW_BUFFER_HIT, stats.RQ_ROW_BUFFER_MISS);
    if (stats.dbus_count_congested > 0)
    {
        std::fprintf(output_statistics.file_handler, " AVG DBUS CONGESTED CYCLE: %.4lf\n", std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested));
    }
    else
    {
        std::fprintf(output_statistics.file_handler, " AVG DBUS CONGESTED CYCLE: -\n");
    }
    std::fprintf(output_statistics.file_handler, "%s WQ ROW_BUFFER_HIT: %10d\n  ROW_BUFFER_MISS: %10d\n  FULL: %10d\n", stats.name.c_str(), stats.WQ_ROW_BUFFER_HIT, stats.WQ_ROW_BUFFER_MISS, stats.WQ_FULL);

#endif // PRINT_STATISTICS_INTO_FILE
}

void champsim::plain_printer::print(champsim::phase_stats& stats)
{
#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "=== {} ===\n", stats.name);
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "=== %s ===\n", stats.name.c_str());
#endif // PRINT_STATISTICS_INTO_FILE

    int i = 0; /** CPU number counter */
    for (auto tn : stats.trace_names)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print(stream, "CPU {} runs {}", i, tn);
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "CPU %d runs %s", i, tn.c_str());
#endif // PRINT_STATISTICS_INTO_FILE

        i++;
    }

    if (NUM_CPUS > 1)
    {
#if (USE_VCPKG == ENABLE)
        fmt::print(stream, "\nTotal Simulation Statistics (not including warmup)\n");
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "\nTotal Simulation Statistics (not including warmup)\n");
#endif // PRINT_STATISTICS_INTO_FILE

        for (const auto& stat : stats.sim_cpu_stats)
        {
            /** @note Call void print(O3_CPU::stats_type); */
            print(stat);
        }

        for (const auto& stat : stats.sim_cache_stats)
        {
            /** @note Call void print(CACHE::stats_type); */
            print(stat);
        }
    }

#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "\nRegion of Interest Statistics\n");
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nRegion of Interest Statistics\n");
#endif // PRINT_STATISTICS_INTO_FILE

    for (const auto& stat : stats.roi_cpu_stats)
    {
        /** @note Call void print(O3_CPU::stats_type); */
        print(stat);
    }

    for (const auto& stat : stats.roi_cache_stats)
    {
        /** @note Call void print(CACHE::stats_type); */
        print(stat);
    }

#if (USE_VCPKG == ENABLE)
    fmt::print(stream, "\nDRAM Statistics\n");
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nDRAM Statistics\n");
#endif // PRINT_STATISTICS_INTO_FILE

    for (const auto& stat : stats.roi_dram_stats)
    {
        /** @note Call void print(DRAM_CHANNEL::stats_type); */
        print(stat);
    }
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
    for (auto p : stats)
    {
        /** @note Call void print(phase_stats& stats); */
        print(p);
    }
}

#else
/* Original code of ChampSim */

void champsim::plain_printer::print(O3_CPU::stats_type stats)
{
    constexpr std::array<std::pair<std::string_view, std::size_t>, 6> types {
        {std::pair {"BRANCH_DIRECT_JUMP", BRANCH_DIRECT_JUMP}, std::pair {"BRANCH_INDIRECT", BRANCH_INDIRECT}, std::pair {"BRANCH_CONDITIONAL", BRANCH_CONDITIONAL},
         std::pair {"BRANCH_DIRECT_CALL", BRANCH_DIRECT_CALL}, std::pair {"BRANCH_INDIRECT_CALL", BRANCH_INDIRECT_CALL},
         std::pair {"BRANCH_RETURN", BRANCH_RETURN}}
    };

    auto total_branch = std::ceil(
        std::accumulate(std::begin(types), std::end(types), 0ll, [tbt = stats.total_branch_types](auto acc, auto next)
            { return acc + tbt[next.second]; }));
    auto total_mispredictions = std::ceil(
        std::accumulate(std::begin(types), std::end(types), 0ll, [btm = stats.branch_type_misses](auto acc, auto next)
            { return acc + btm[next.second]; }));

    fmt::print(stream, "\n{} cumulative IPC: {:.4g} instructions: {} cycles: {}\n", stats.name, std::ceil(stats.instrs()) / std::ceil(stats.cycles()),
        stats.instrs(), stats.cycles());
    fmt::print(stream, "{} Branch Prediction Accuracy: {:.4g}% MPKI: {:.4g} Average ROB Occupancy at Mispredict: {:.4g}\n", stats.name,
        (100.0 * std::ceil(total_branch - total_mispredictions)) / total_branch, (1000.0 * total_mispredictions) / std::ceil(stats.instrs()),
        std::ceil(stats.total_rob_occupancy_at_branch_mispredict) / total_mispredictions);

    std::vector<double> mpkis;
    std::transform(std::begin(stats.branch_type_misses), std::end(stats.branch_type_misses), std::back_inserter(mpkis),
        [instrs = stats.instrs()](auto x)
        { return 1000.0 * std::ceil(x) / std::ceil(instrs); });

    fmt::print(stream, "Branch type MPKI\n");
    for (auto [str, idx] : types)
        fmt::print(stream, "{}: {:.3}\n", str, mpkis[idx]);
    fmt::print(stream, "\n");
}

void champsim::plain_printer::print(CACHE::stats_type stats)
{
    constexpr std::array<std::pair<std::string_view, std::size_t>, 5> types {
        {std::pair {"LOAD", champsim::to_underlying(access_type::LOAD)}, std::pair {"RFO", champsim::to_underlying(access_type::RFO)},
         std::pair {"PREFETCH", champsim::to_underlying(access_type::PREFETCH)}, std::pair {"WRITE", champsim::to_underlying(access_type::WRITE)},
         std::pair {"TRANSLATION", champsim::to_underlying(access_type::TRANSLATION)}}
    };

    for (std::size_t cpu = 0; cpu < NUM_CPUS; ++cpu)
    {
        uint64_t TOTAL_HIT = 0, TOTAL_MISS = 0;
        for (const auto& type : types)
        {
            TOTAL_HIT += stats.hits.at(type.second).at(cpu);
            TOTAL_MISS += stats.misses.at(type.second).at(cpu);
        }

        fmt::print(stream, "{} TOTAL        ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, TOTAL_HIT + TOTAL_MISS, TOTAL_HIT, TOTAL_MISS);
        for (const auto& type : types)
        {
            fmt::print(stream, "{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d}\n", stats.name, type.first,
                stats.hits[type.second][cpu] + stats.misses[type.second][cpu], stats.hits[type.second][cpu], stats.misses[type.second][cpu]);
        }

        fmt::print(stream, "{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}\n", stats.name, stats.pf_requested, stats.pf_issued,
            stats.pf_useful, stats.pf_useless);

        fmt::print(stream, "{} AVERAGE MISS LATENCY: {:.4g} cycles\n", stats.name, stats.avg_miss_latency);
    }
}

void champsim::plain_printer::print(DRAM_CHANNEL::stats_type stats)
{
    fmt::print(stream, "\n{} RQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n", stats.name, stats.RQ_ROW_BUFFER_HIT, stats.RQ_ROW_BUFFER_MISS);
    if (stats.dbus_count_congested > 0)
        fmt::print(stream, " AVG DBUS CONGESTED CYCLE: {:.4g}\n", std::ceil(stats.dbus_cycle_congested) / std::ceil(stats.dbus_count_congested));
    else
        fmt::print(stream, " AVG DBUS CONGESTED CYCLE: -\n");
    fmt::print(stream, "WQ ROW_BUFFER_HIT: {:10}\n  ROW_BUFFER_MISS: {:10}\n  FULL: {:10}\n", stats.name, stats.WQ_ROW_BUFFER_HIT, stats.WQ_ROW_BUFFER_MISS,
        stats.WQ_FULL);
}

void champsim::plain_printer::print(champsim::phase_stats& stats)
{
    fmt::print(stream, "=== {} ===\n", stats.name);

    int i = 0;
    for (auto tn : stats.trace_names)
        fmt::print(stream, "CPU {} runs {}", i++, tn);

    if (NUM_CPUS > 1)
    {
        fmt::print(stream, "\nTotal Simulation Statistics (not including warmup)\n");

        for (const auto& stat : stats.sim_cpu_stats)
            print(stat);

        for (const auto& stat : stats.sim_cache_stats)
            print(stat);
    }

    fmt::print(stream, "\nRegion of Interest Statistics\n");

    for (const auto& stat : stats.roi_cpu_stats)
        print(stat);

    for (const auto& stat : stats.roi_cache_stats)
        print(stat);

    fmt::print(stream, "\nDRAM Statistics\n");
    for (const auto& stat : stats.roi_dram_stats)
        print(stat);
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
    for (auto p : stats)
        print(p);
}

#endif // USER_CODES

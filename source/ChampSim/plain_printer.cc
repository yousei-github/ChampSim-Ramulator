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
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#endif /* USE_VCPKG */

#include <cmath>
#include <numeric>
#include <ratio>
#include <string_view> // for string_view
#include <utility>
#include <vector>

#include "ChampSim/stats_printer.h"

namespace
{
template<typename N, typename D>
auto print_ratio(N num, D denom)
{
    if (denom > 0)
    {
#if (USE_VCPKG == ENABLE)
        return fmt::format("{:.4g}", std::ceil(num) / std::ceil(denom));
#endif /* USE_VCPKG */
    }
    return std::string {"-"};
}
} // namespace

std::vector<std::string> champsim::plain_printer::format(O3_CPU::stats_type stats)
{
    constexpr std::array types {branch_type::BRANCH_DIRECT_JUMP, branch_type::BRANCH_INDIRECT, branch_type::BRANCH_CONDITIONAL, branch_type::BRANCH_DIRECT_CALL, branch_type::BRANCH_INDIRECT_CALL, branch_type::BRANCH_RETURN};
    auto total_branch         = std::ceil(std::accumulate(std::begin(types), std::end(types), 0LL, [tbt = stats.total_branch_types](auto acc, auto next)
                { return acc + tbt.value_or(next, 0); }));
    auto total_mispredictions = std::ceil(std::accumulate(std::begin(types), std::end(types), 0LL, [btm = stats.branch_type_misses](auto acc, auto next)
        { return acc + btm.value_or(next, 0); }));

    std::vector<std::string> lines {};

#if (USE_VCPKG == ENABLE)
    lines.push_back(fmt::format("{} cumulative IPC: {} instructions: {} cycles: {}", stats.name, ::print_ratio(stats.instrs(), stats.cycles()), stats.instrs(), stats.cycles()));

    lines.push_back(fmt::format("{} Branch Prediction Accuracy: {}% MPKI: {} Average ROB Occupancy at Mispredict: {}", stats.name,
        ::print_ratio(100 * (total_branch - total_mispredictions), total_branch),
        ::print_ratio(std::kilo::num * total_mispredictions, stats.instrs()),
        ::print_ratio(stats.total_rob_occupancy_at_branch_mispredict, total_mispredictions)));

    lines.emplace_back("Branch type MPKI");
    for (auto idx : types)
    {
        lines.push_back(fmt::format("{}: {}", branch_type_names.at(champsim::to_underlying(idx)), ::print_ratio(std::kilo::num * stats.branch_type_misses.value_or(idx, 0), stats.instrs())));
    }
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n");

    std::fprintf(output_statistics.file_handler, "%s cumulative IPC: %s instructions: %lld cycles: %lld\n",
        stats.name.c_str(), ::print_ratio(stats.instrs(), stats.cycles()).c_str(), stats.instrs(), stats.cycles());
    std::fprintf(output_statistics.file_handler, "%s Branch Prediction Accuracy: %s%% MPKI: %s Average ROB Occupancy at Mispredict: %s\n",
        stats.name.c_str(),
        ::print_ratio(100 * (total_branch - total_mispredictions), total_branch).c_str(),
        ::print_ratio(std::kilo::num * total_mispredictions, stats.instrs()).c_str(),
        ::print_ratio(stats.total_rob_occupancy_at_branch_mispredict, total_mispredictions).c_str());

    std::fprintf(output_statistics.file_handler, "Branch type MPKI\n");
    for (auto idx : types)
    {
        std::fprintf(output_statistics.file_handler, "%s: %s\n",
            branch_type_names.at(champsim::to_underlying(idx)).data(), ::print_ratio(std::kilo::num * stats.branch_type_misses.value_or(idx, 0), stats.instrs()).c_str());
    }
#endif /* PRINT_STATISTICS_INTO_FILE */

    return lines;
}

std::vector<std::string> champsim::plain_printer::format(CACHE::stats_type stats)
{
    using hits_value_type        = typename decltype(stats.hits)::value_type;
    using misses_value_type      = typename decltype(stats.misses)::value_type;
    using mshr_merge_value_type  = typename decltype(stats.mshr_merge)::value_type;
    using mshr_return_value_type = typename decltype(stats.mshr_return)::value_type;

    std::vector<std::size_t> cpus;

    // build a vector of all existing cpus
    auto stat_keys = {stats.hits.get_keys(), stats.misses.get_keys(), stats.mshr_merge.get_keys(), stats.mshr_return.get_keys()};
    for (auto keys : stat_keys)
    {
        std::transform(std::begin(keys), std::end(keys), std::back_inserter(cpus), [](auto val)
            { return val.second; });
    }
    std::sort(std::begin(cpus), std::end(cpus));
    auto uniq_end = std::unique(std::begin(cpus), std::end(cpus));
    cpus.erase(uniq_end, std::end(cpus));

    for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION})
    {
        for (auto cpu : cpus)
        {
            stats.hits.allocate(std::pair {type, cpu});
            stats.misses.allocate(std::pair {type, cpu});
            stats.mshr_merge.allocate(std::pair {type, cpu});
            stats.mshr_return.allocate(std::pair {type, cpu});
        }
    }

    std::vector<std::string> lines {};

    for (auto cpu : cpus)
    {
#if (USE_VCPKG == ENABLE) || (PRINT_STATISTICS_INTO_FILE == ENABLE)
        hits_value_type total_hits               = 0;
        misses_value_type total_misses           = 0;
        mshr_merge_value_type total_mshr_merge   = 0;
        mshr_return_value_type total_mshr_return = 0;
        for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION})
        {
            total_hits += stats.hits.value_or(std::pair {type, cpu}, hits_value_type {});
            total_misses += stats.misses.value_or(std::pair {type, cpu}, misses_value_type {});
            total_mshr_merge += stats.mshr_merge.value_or(std::pair {type, cpu}, mshr_merge_value_type {});
            total_mshr_return += stats.mshr_return.value_or(std::pair {type, cpu}, mshr_merge_value_type {});
        }
        uint64_t total_downstream_demands = total_mshr_return - stats.mshr_return.value_or(std::pair {access_type::PREFETCH, cpu}, mshr_return_value_type {});
#endif /* USE_VCPKG, PRINT_STATISTICS_INTO_FILE */

#if (USE_VCPKG == ENABLE)
        fmt::format_string<std::string_view, std::string_view, int, int, int> hitmiss_fmtstr {"cpu{}->{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d} MSHR_MERGE: {:10d}"};

        lines.push_back(fmt::format(hitmiss_fmtstr, cpu, stats.name, "TOTAL", total_hits + total_misses, total_hits, total_misses, total_mshr_merge));
        for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION})
        {
            lines.push_back(fmt::format(hitmiss_fmtstr, cpu, stats.name, access_type_names.at(champsim::to_underlying(type)),
                stats.hits.value_or(std::pair {type, cpu}, hits_value_type {}) + stats.misses.value_or(std::pair {type, cpu}, misses_value_type {}),
                stats.hits.value_or(std::pair {type, cpu}, hits_value_type {}), stats.misses.value_or(std::pair {type, cpu}, misses_value_type {}),
                stats.mshr_merge.value_or(std::pair {type, cpu}, mshr_merge_value_type {})));
        }

        lines.push_back(fmt::format("cpu{}->{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}", cpu, stats.name, stats.pf_requested, stats.pf_issued, stats.pf_useful, stats.pf_useless));
        lines.push_back(fmt::format("cpu{}->{} AVERAGE MISS LATENCY: {} cycles", cpu, stats.name, ::print_ratio(stats.total_miss_latency_cycles, total_downstream_demands)));
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "\n");

        const char hitmiss_str[] = "cpu%ld->%s %-12s ACCESS: %10ld HIT: %10ld MISS: %10ld MSHR_MERGE: %10ld\n";

        std::fprintf(output_statistics.file_handler, hitmiss_str, cpu, stats.name.c_str(), "TOTAL", total_hits + total_misses, total_hits, total_misses, total_mshr_merge);
        for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION})
        {
            std::fprintf(output_statistics.file_handler, hitmiss_str,
                cpu, stats.name.c_str(), access_type_names.at(champsim::to_underlying(type)).data(),
                stats.hits.value_or(std::pair {type, cpu}, hits_value_type {}) + stats.misses.value_or(std::pair {type, cpu}, misses_value_type {}),
                stats.hits.value_or(std::pair {type, cpu}, hits_value_type {}),
                stats.misses.value_or(std::pair {type, cpu}, misses_value_type {}),
                stats.mshr_merge.value_or(std::pair {type, cpu}, mshr_merge_value_type {}));
        }

        std::fprintf(output_statistics.file_handler, "cpu%ld->%s PREFETCH REQUESTED: %10ld ISSUED: %10ld USEFUL: %10ld USELESS: %10ld\n",
            cpu, stats.name.c_str(), stats.pf_requested, stats.pf_issued, stats.pf_useful, stats.pf_useless);
        std::fprintf(output_statistics.file_handler, "cpu%ld->%s AVERAGE MISS LATENCY: %s cycles\n",
            cpu, stats.name.c_str(), ::print_ratio(stats.total_miss_latency_cycles, total_downstream_demands).c_str());
#endif /* PRINT_STATISTICS_INTO_FILE */
    }

    return lines;
}

std::vector<std::string> champsim::plain_printer::format(DRAM_CHANNEL::stats_type stats)
{
    std::vector<std::string> lines {};

#if (USE_VCPKG == ENABLE)
    lines.push_back(fmt::format("{} RQ ROW_BUFFER_HIT: {:10}", stats.name, stats.RQ_ROW_BUFFER_HIT));
    lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.RQ_ROW_BUFFER_MISS));
    lines.push_back(fmt::format("  AVG DBUS CONGESTED CYCLE: {}", ::print_ratio(stats.dbus_cycle_congested, stats.dbus_count_congested)));
    lines.push_back(fmt::format("{} WQ ROW_BUFFER_HIT: {:10}", stats.name, stats.WQ_ROW_BUFFER_HIT));
    lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.WQ_ROW_BUFFER_MISS));
    lines.push_back(fmt::format("  FULL: {:10}", stats.WQ_FULL));

    if (stats.refresh_cycles > 0)
        lines.push_back(fmt::format("{} REFRESHES ISSUED: {:10}", stats.name, stats.refresh_cycles));
    else
        lines.push_back(fmt::format("{} REFRESHES ISSUED: -", stats.name));
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n");

    std::fprintf(output_statistics.file_handler, "%s RQ ROW_BUFFER_HIT: %10d\n", stats.name.c_str(), stats.RQ_ROW_BUFFER_HIT);
    std::fprintf(output_statistics.file_handler, "  ROW_BUFFER_MISS: %10d\n", stats.RQ_ROW_BUFFER_MISS);
    std::fprintf(output_statistics.file_handler, "  AVG DBUS CONGESTED CYCLE: %s\n", ::print_ratio(stats.dbus_cycle_congested, stats.dbus_count_congested).c_str());
    std::fprintf(output_statistics.file_handler, "%s WQ ROW_BUFFER_HIT: %10d\n", stats.name.c_str(), stats.WQ_ROW_BUFFER_HIT);
    std::fprintf(output_statistics.file_handler, "  ROW_BUFFER_MISS: %10d\n", stats.WQ_ROW_BUFFER_MISS);
    std::fprintf(output_statistics.file_handler, "  FULL: %10d\n", stats.WQ_FULL);

    if (stats.refresh_cycles > 0)
        std::fprintf(output_statistics.file_handler, "%s REFRESHES ISSUED: %10ld\n", stats.name.c_str(), stats.refresh_cycles);
    else
        std::fprintf(output_statistics.file_handler, "%s REFRESHES ISSUED: -\n", stats.name.c_str());
#endif /* PRINT_STATISTICS_INTO_FILE */

    return lines;
}

std::vector<std::string> champsim::plain_printer::format(champsim::phase_stats& stats)
{
    std::vector<std::string> lines {};

#if (USE_VCPKG == ENABLE)
    lines.push_back(fmt::format("=== {} ===", stats.name));
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "=== %s ===\n", stats.name.c_str());
#endif /* PRINT_STATISTICS_INTO_FILE */

    int i = 0;
    for (auto tn : stats.trace_names)
    {
#if (USE_VCPKG == ENABLE)
        lines.push_back(fmt::format("CPU {} runs {}", i, tn));
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "CPU %d runs %s\n", i, tn.c_str());
#endif /* PRINT_STATISTICS_INTO_FILE */

        i++;
    }

    if (NUM_CPUS > 1)
    {
        lines.emplace_back("");
        lines.emplace_back("Total Simulation Statistics (not including warmup)");

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "\nTotal Simulation Statistics (not including warmup)\n");
#endif /* PRINT_STATISTICS_INTO_FILE */

        for (const auto& stat : stats.sim_cpu_stats)
        {
            auto sublines = format(stat);
            lines.emplace_back("");
            std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
            lines.emplace_back("");

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
            std::fprintf(output_statistics.file_handler, "\n\n");
#endif /* PRINT_STATISTICS_INTO_FILE */
        }

        for (const auto& stat : stats.sim_cache_stats)
        {
            auto sublines = format(stat);
            std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
        }
    }

    lines.emplace_back("");
    lines.emplace_back("Region of Interest Statistics");

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nRegion of Interest Statistics\n");
#endif /* PRINT_STATISTICS_INTO_FILE */

    for (const auto& stat : stats.roi_cpu_stats)
    {
        auto sublines = format(stat);
        lines.emplace_back("");
        std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
        lines.emplace_back("");

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "\n\n");
#endif /* PRINT_STATISTICS_INTO_FILE */
    }

    for (const auto& stat : stats.roi_cache_stats)
    {
        auto sublines = format(stat);
        std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
    }

    lines.emplace_back("");
    lines.emplace_back("DRAM Statistics");

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nDRAM Statistics\n");
#endif /* PRINT_STATISTICS_INTO_FILE */

    for (const auto& stat : stats.roi_dram_stats)
    {
        auto sublines = format(stat);
        lines.emplace_back("");
        std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        std::fprintf(output_statistics.file_handler, "\n");
#endif /* PRINT_STATISTICS_INTO_FILE */
    }

    return lines;
}

void champsim::plain_printer::print(champsim::phase_stats& stats)
{
    auto lines = format(stats);
    std::copy(std::begin(lines), std::end(lines), std::ostream_iterator<std::string>(stream, "\n"));
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
    for (auto p : stats)
    {
        print(p);
    }
}

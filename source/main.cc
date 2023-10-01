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

/* Header */
#include "main.h"

/* Macro */

/* Type */
typedef int argc_type;

struct simulator_input_parameter
{
    bool knob_cloudsuite {false};
    bool hide_given {false};
    bool warmup_given {false};
    bool simulation_given {false};
    uint64_t warmup_instructions     = 0;
    uint64_t simulation_instructions = std::numeric_limits<uint64_t>::max();

    bool json_given {false};
    std::string json_file_name;
    std::vector<std::string> trace_names;

    std::vector<champsim::phase_info> phases;
    std::vector<champsim::tracereader> traces;
};

/* Prototype */
// For ChampSim
namespace champsim::configured
{
template<typename MEMORY_TYPE>
struct generated_environment final : public champsim::environment
{
    champsim::channel LLC_to_MEMORY_CONTROLLER_queues {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max(), champsim::lg2(BLOCK_SIZE), 0};
    champsim::channel cpu0_STLB_to_cpu0_PTW_queues {32, 0, 0, champsim::lg2(PAGE_SIZE), 0};
    champsim::channel cpu0_DTLB_to_cpu0_STLB_queues {32, 32, 32, champsim::lg2(4096), 0};
    champsim::channel cpu0_ITLB_to_cpu0_STLB_queues {32, 32, 32, champsim::lg2(4096), 0};
    champsim::channel cpu0_L2C_to_cpu0_STLB_queues {32, 32, 32, champsim::lg2(4096), 0};
    champsim::channel cpu0_L1D_to_cpu0_L2C_queues {32, 32, 32, champsim::lg2(64), 0};
    champsim::channel cpu0_L1I_to_cpu0_L2C_queues {32, 32, 32, champsim::lg2(64), 0};
    champsim::channel cpu0_to_cpu0_L1I_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_PTW_to_cpu0_L1D_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_to_cpu0_L1D_queues {32, 32, 32, champsim::lg2(64), 1};
    champsim::channel cpu0_L1I_to_cpu0_ITLB_queues {16, 16, 16, champsim::lg2(4096), 1};
    champsim::channel cpu0_L1D_to_cpu0_DTLB_queues {16, 16, 16, champsim::lg2(4096), 1};
    champsim::channel cpu0_L2C_to_LLC_queues {32, 32, 32, champsim::lg2(64), 0};

    MEMORY_CONTROLLER<MEMORY_TYPE>& memory_controller;
    //MEMORY_CONTROLLER<MEMORY_TYPE> memory_controller {1.25, 3200, 12.5, 12.5, 12.5, 7.5, {&LLC_to_MEMORY_CONTROLLER_queues}}; // MEMORY_CONTROLLER(double freq_scale, double clock_scale, std::vector<channel_type*>&& ul, Memory<T, Controller>& memory);
    VirtualMemory vmem {4096, 5, 200, memory_controller.size()};
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
                   .lower_level(&LLC_to_MEMORY_CONTROLLER_queues)};

    CACHE cpu0_DTLB {CACHE::Builder {champsim::defaults::default_dtlb}
                         .name("cpu0_DTLB")
                         .frequency(1.0)
                         .sets(16)
                         .pq_size(16)
                         .mshr_size(8)
                         .tag_bandwidth(1)
                         .fill_bandwidth(1)
                         .offset_bits(champsim::lg2(4096))
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
                         .offset_bits(champsim::lg2(4096))
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
            .offset_bits(champsim::lg2(4096))
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

    generated_environment(ramulator::Memory<MEMORY_TYPE, ramulator::Controller>& memory)
    {
        memory_controller = new MEMORY_CONTROLLER<MEMORY_TYPE>(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, memory);
    }

    ~generated_environment()
    {
        delete memory_controller;
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

    MEMORY_CONTROLLER& dram_view() override { return memory_controller; }

    std::vector<std::reference_wrapper<champsim::operable>> operable_view() override
    {
        return {cpu0, cpu0_PTW, LLC, cpu0_DTLB, cpu0_ITLB, cpu0_L1D, cpu0_L1I, cpu0_L2C, cpu0_STLB, memory_controller};
    }
};
} // namespace champsim::configured

namespace champsim
{
std::vector<phase_stats> main(environment& env, std::vector<phase_info>& phases, std::vector<tracereader>& traces);
} // namespace champsim

#if (RAMULATOR == ENABLE)
// For Ramulator
using namespace ramulator;

#if (MEMORY_USE_HYBRID == ENABLE)
void configure_fast_memory_to_run_simulation(const std::string& standard, Config& configs, const std::string& standard2, Config& configs2);

template<typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, Config& configs2, Config& configs, T* spec);

template<typename T, typename T2>
void start_run_simulation(const Config& configs, T* spec, const Config& configs2, T2* spec2);

template<typename T, typename T2>
void simulation_run(const Config& configs, Memory<T, Controller>& memory, const Config& configs2, Memory<T2, Controller>& memory2);
#else
void configure_memory_to_run_simulation(const std::string& standard, Config& configs, simulator_input_parameter& input_parameter);

template<typename T>
void start_run_simulation(const Config& configs, T* spec, simulator_input_parameter& input_parameter);

template<typename T>
void simulation_run(const Config& configs, Memory<T, Controller>& memory, simulator_input_parameter& input_parameter);
#endif // MEMORY_USE_HYBRID
#else
void simulation_run();
#endif // RAMULATOR

/* Variable */

/* Function */
#if (USER_CODES == ENABLE)
int main(int argc, char** argv)
{
    /** @note Test the OpenMP functionality */
#if (USE_OPENMP == ENABLE)
    omp_set_num_threads(SET_THREADS_NUMBER);
#pragma omp parallel
    {
        // show how many cores your computer have
        printf("(%s) Thread %d of %d threads says hello.\n", __func__, omp_get_thread_num(), omp_get_num_threads());
    }
#endif // USE_OPENMP

    CLI::App app {"A microarchitecture simulator for research and education"};

    simulator_input_parameter input_parameter;

    // bool knob_cloudsuite {false};
    // bool hide_given {false};
    // bool warmup_given {false};
    // bool simulation_given {false};
    // uint64_t warmup_instructions     = 0;
    // uint64_t simulation_instructions = std::numeric_limits<uint64_t>::max();

    // bool json_given {false};
    // std::string json_file_name;
    // std::vector<std::string> trace_names;

    /** @note Process the input parameters */
    if (argc < 2)
    {
#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
        printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> [--stats <filename>] <configs-file> <configs-file2> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 ramulator-configs1.cfg ramulator-configs2.cfg cpu_trace.xz\n",
            argv[0], argv[0]);
#else
        printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> [--stats <filename>] <configs-file> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 ramulator-configs1.cfg cpu_trace.xz\n",
            argv[0], argv[0]);
#endif // MEMORY_USE_HYBRID
#else
        printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 cpu_trace.xz\n",
            argv[0], argv[0]);
#endif // RAMULATOR

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
        printf("\nNote: assign cpu traces to each cpu by appending multiple trace files to the command line.\n");
#endif // CPU_USE_MULTIPLE_CORES

        return EXIT_SUCCESS;
    }

    argc_type start_position_of_traces = 0;
#if (RAMULATOR == ENABLE)
    argc_type start_position_of_configs = 0;
    string stats_out;
    bool stats_flag {false};
    bool mapping_flag {false};
    argc_type start_position_of_stats   = 0;
    argc_type start_position_of_mapping = 0;
#endif // RAMULATOR
    for (auto i = 1; i < argc; i++)
    {
        static uint8_t abort_flag = 0;

        /** Read all traces using the cloudsuite format */
        if ((strcmp(argv[i], "--cloudsuite") == 0) || (strcmp(argv[i], "-c") == 0))
        {
            input_parameter.knob_cloudsuite = true;

            start_position_of_traces++;
            continue;
        }

        /** Hide the heartbeat output */
        if (strcmp(argv[i], "--hide-heartbeat") == 0)
        {
            input_parameter.hide_given = true;

            start_position_of_traces++;
            continue;
        }

        /** The number of instructions in the warmup phase */
        if ((strcmp(argv[i], "--warmup-instructions") == 0) || (strcmp(argv[i], "-w") == 0))
        {
            input_parameter.warmup_given = true;
            if (i + 1 < argc)
            {
                input_parameter.warmup_instructions = atol(argv[++i]);

#if (RAMULATOR == ENABLE)
                start_position_of_configs = i + 1;
                start_position_of_traces  = start_position_of_configs + NUMBER_OF_MEMORIES;
#else
                start_position_of_traces = i + 1;
#endif // RAMULATOR
                continue;
            }
            else
            {
                std::cout << __func__ << ": Need parameter behind --warmup-instructions or -w." << std::endl;
                abort_flag++;
            }
        }

        /** The number of instructions in the detailed phase. If not specified, run to the end of the trace */
        if ((strcmp(argv[i], "--simulation-instructions") == 0) || (strcmp(argv[i], "-i") == 0))
        {
            input_parameter.simulation_given = true;
            if (i + 1 < argc)
            {
                input_parameter.simulation_instructions = atol(argv[++i]);

#if (RAMULATOR == ENABLE)
                start_position_of_configs = i + 1;
                start_position_of_traces  = start_position_of_configs + NUMBER_OF_MEMORIES;
#else
                start_position_of_traces = i + 1;
#endif // RAMULATOR
                continue;
            }
            else
            {
                std::cout << __func__ << ": Need parameter behind --simulation-instructions or -i." << std::endl;
                abort_flag++;
            }
        }

        /** The name of the file to receive JSON output. If no name is specified, stdout will be used */
        if (strcmp(argv[i], "--json") == 0)
        {
            input_parameter.json_given = true;
            if (i + 1 < argc)
            {
                input_parameter.json_file_name = argv[++i];

#if (RAMULATOR == ENABLE)
                start_position_of_configs = i + 1;
                start_position_of_traces  = start_position_of_configs + NUMBER_OF_MEMORIES;
#else
                start_position_of_traces = i + 1;
#endif // RAMULATOR
                continue;
            }
            else
            {
                std::cout << __func__ << ": Need parameter behind --json." << std::endl;
                abort_flag++;
            }
        }

#if (RAMULATOR == ENABLE)
        if (strcmp(argv[i], "--stats") == 0)
        {
            if (i + 1 < argc)
            {
                stats_flag                = true;
                start_position_of_stats   = ++i;

                start_position_of_configs = i + 1;
                start_position_of_traces  = start_position_of_configs + NUMBER_OF_MEMORIES;
                continue;
            }
            else
            {
                std::cout << __func__ << ": Need parameter behind --stats." << std::endl;
                abort_flag++;
            }
        }

        if (strcmp(argv[i], "--mapping") == 0)
        {
            if (i + 1 < argc)
            {
                mapping_flag              = true;
                start_position_of_mapping = ++i;

                start_position_of_configs = i + 1;
                start_position_of_traces  = start_position_of_configs + NUMBER_OF_MEMORIES;
                continue;
            }
            else
            {
                std::cout << __func__ << ": Need parameter behind --mapping." << std::endl;
                abort_flag++;
            }
        }
#endif // RAMULATOR

        /** Input parameter error detection */
        if (abort_flag)
        {
            std::printf("%s: Input parameter error detection! abort_flag=%d\n", __FUNCTION__, abort_flag);
            abort();
        }
    }

    /** @note Store the trace names */
    for (argc_type i = start_position_of_traces; i < argc; i++) // From argv[start_position_of_traces] to argv[argc - start_position_of_traces - 1]
    {
        input_parameter.trace_names.push_back(argv[i]);
    }

    if (input_parameter.trace_names.size() > NUM_CPUS)
    {
        std::printf("\n*** Too many traces for the configured number of cores ***\n\n");
        assert(false);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // Prepare file for recording memory traces.
    output_memorytrace.output_file_initialization(&(argv[start_position_of_traces]), argc - start_position_of_traces);
#endif // PRINT_MEMORY_TRACE

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    // Prepare file for recording statistics.
    output_statistics.output_file_initialization(&(argv[start_position_of_traces]), argc - start_position_of_traces);
#endif // PRINT_STATISTICS_INTO_FILE

    /** @note Prepare the ChampSim framework */
    if (input_parameter.simulation_given && ! input_parameter.warmup_given)
        input_parameter.warmup_instructions = input_parameter.simulation_instructions * 2 / 10;

    //std::vector<champsim::tracereader> traces;
    std::transform(std::begin(input_parameter.trace_names), std::end(input_parameter.trace_names), std::back_inserter(input_parameter.traces),
        [knob_cloudsuite = input_parameter.knob_cloudsuite, repeat = input_parameter.simulation_given, i = uint8_t(0)](auto name) mutable
        { return get_tracereader(name, i++, knob_cloudsuite, repeat); });

    // std::vector<champsim::phase_info> phases {
    //     {champsim::phase_info {"Warmup", true, warmup_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names},
    //      champsim::phase_info {"Simulation", false, simulation_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names}}
    // };
    input_parameter.phases.push_back(champsim::phase_info {"Warmup", true, input_parameter.warmup_instructions, std::vector<std::size_t>(std::size(input_parameter.trace_names), 0), input_parameter.trace_names});
    input_parameter.phases.push_back(champsim::phase_info {"Simulation", false, input_parameter.simulation_instructions, std::vector<std::size_t>(std::size(input_parameter.trace_names), 0), input_parameter.trace_names});

    for (auto& p : input_parameter.phases)
    {
        std::iota(std::begin(p.trace_index), std::end(p.trace_index), 0);
    }

    /** @note Prepare the Ramulator framework */
#if (RAMULATOR == ENABLE)
    // Prepare initialization for Ramulator.
#if (MEMORY_USE_HYBRID == ENABLE)
    // Read memory configuration file.
    Config configs(argv[start_position_of_configs]);      // Fast memory
    Config configs2(argv[start_position_of_configs + 1]); // Slow memory

    const std::string& standard = configs["standard"];
    assert(standard != "" || "DRAM standard should be specified.");
    const std::string& standard2 = configs2["standard"];
    assert(standard2 != "" || "DRAM standard should be specified.");

    configs.add("trace_type", "DRAM");
    configs2.add("trace_type", "DRAM");

    if (stats_flag == false)
    {
        // Open .stats file.
        string stats_file_name = standard + "_" + standard2;
        Stats::statlist.output(stats_file_name + ".stats");
        stats_out = stats_file_name + string(".stats");
    }
    else
    {
        Stats::statlist.output(argv[start_position_of_stats]);
        stats_out = argv[start_position_of_stats];
    }

    // A separate file defines mapping for easy config.
    if (mapping_flag == false)
    {
        configs.add("mapping", "defaultmapping");
        configs2.add("mapping", "defaultmapping");
    }
    else
    {
        configs.add("mapping", argv[start_position_of_mapping]);
        configs2.add("mapping", argv[start_position_of_mapping]);
    }

    configs.set_core_num(NUM_CPUS);
    configs2.set_core_num(NUM_CPUS);
#else
    // Read memory configuration file.
    Config configs(argv[start_position_of_configs]);

    const std::string& standard = configs["standard"];
    assert(standard != "" || "DRAM standard should be specified.");

    configs.add("trace_type", "DRAM");

    if (stats_flag == false)
    {
        // Open .stats file.
        string stats_file_name = standard;
        Stats::statlist.output(stats_file_name + ".stats");
        stats_out = stats_file_name + string(".stats");
    }
    else
    {
        Stats::statlist.output(argv[start_position_of_stats]);
        stats_out = argv[start_position_of_stats];
    }

    // A separate file defines mapping for easy config.
    if (mapping_flag == false)
    {
        configs.add("mapping", "defaultmapping");
    }
    else
    {
        configs.add("mapping", argv[start_position_of_mapping]);
    }

    configs.set_core_num(NUM_CPUS);
#endif // MEMORY_USE_HYBRID
#endif // RAMULATOR

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
    configure_fast_memory_to_run_simulation(standard, configs, standard2, configs2);
#else
    configure_memory_to_run_simulation(standard, configs, input_parameter);
#endif // MEMORY_USE_HYBRID

    std::printf("Simulation done. Statistics written to %s\n", stats_out.c_str());
#else
    simulation_run();
    std::printf("Simulation done.\n");
#endif // RAMULATOR

    return EXIT_SUCCESS;
}

#else
int main(int argc, char** argv)
{
    champsim::configured::generated_environment gen_environment {};

    CLI::App app {"A microarchitecture simulator for research and education"};

    bool knob_cloudsuite {false};
    uint64_t warmup_instructions     = 0;
    uint64_t simulation_instructions = std::numeric_limits<uint64_t>::max();
    std::string json_file_name;
    std::vector<std::string> trace_names;

    auto set_heartbeat_callback = [&](auto)
    {
        for (O3_CPU& cpu : gen_environment.cpu_view())
            cpu.show_heartbeat = false;
    };

    app.add_flag("-c,--cloudsuite", knob_cloudsuite, "Read all traces using the cloudsuite format");
    app.add_flag("--hide-heartbeat", set_heartbeat_callback, "Hide the heartbeat output");
    auto warmup_instr_option        = app.add_option("-w,--warmup-instructions", warmup_instructions, "The number of instructions in the warmup phase");
    auto deprec_warmup_instr_option = app.add_option("--warmup-instructions", warmup_instructions, "[deprecated] use --warmup-instructions instead")->excludes(warmup_instr_option);
    auto sim_instr_option           = app.add_option("-i,--simulation-instructions", simulation_instructions, "The number of instructions in the detailed phase. If not specified, run to the end of the trace.");
    auto deprec_sim_instr_option    = app.add_option("--simulation-instructions", simulation_instructions, "[deprecated] use --simulation-instructions instead")->excludes(sim_instr_option);

    auto json_option                = app.add_option("--json", json_file_name, "The name of the file to receive JSON output. If no name is specified, stdout will be used")->expected(0, 1);

    app.add_option("traces", trace_names, "The paths to the traces")->required()->expected(NUM_CPUS)->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    const bool warmup_given     = (warmup_instr_option->count() > 0) || (deprec_warmup_instr_option->count() > 0);
    const bool simulation_given = (sim_instr_option->count() > 0) || (deprec_sim_instr_option->count() > 0);

    if (deprec_warmup_instr_option->count() > 0)
        fmt::print("WARNING: option --warmup-instructions is deprecated. Use --warmup-instructions instead.\n");

    if (deprec_sim_instr_option->count() > 0)
        fmt::print("WARNING: option --simulation-instructions is deprecated. Use --simulation-instructions instead.\n");

    if (simulation_given && ! warmup_given)
        warmup_instructions = simulation_instructions * 2 / 10;

    std::vector<champsim::tracereader> traces;
    std::transform(std::begin(trace_names), std::end(trace_names), std::back_inserter(traces),
        [knob_cloudsuite, repeat = simulation_given, i = uint8_t(0)](auto name) mutable
        { return get_tracereader(name, i++, knob_cloudsuite, repeat); });

    std::vector<champsim::phase_info> phases {
        {champsim::phase_info {"Warmup", true, warmup_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names},
         champsim::phase_info {"Simulation", false, simulation_instructions, std::vector<std::size_t>(std::size(trace_names), 0), trace_names}}
    };

    for (auto& p : phases)
        std::iota(std::begin(p.trace_index), std::end(p.trace_index), 0);

    fmt::print("\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: {}\nSimulation Instructions: {}\nNumber of CPUs: {}\nPage size: {}\n\n", phases.at(0).length, phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);

    auto phase_stats = champsim::main(gen_environment, phases, traces);

    fmt::print("\nChampSim completed all CPUs\n\n");

    champsim::plain_printer {std::cout}.print(phase_stats);

    for (CACHE& cache : gen_environment.cache_view())
        cache.impl_prefetcher_final_stats();

    for (CACHE& cache : gen_environment.cache_view())
        cache.impl_replacement_final_stats();

    if (json_option->count() > 0)
    {
        if (json_file_name.empty())
        {
            champsim::json_printer {std::cout}.print(phase_stats);
        }
        else
        {
            std::ofstream json_file {json_file_name};
            champsim::json_printer {json_file}.print(phase_stats);
        }
    }

    return EXIT_SUCCESS;
}

#endif // USER_CODES

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)

void configure_fast_memory_to_run_simulation(const std::string& standard, Config& configs,
    const std::string& standard2, Config& configs2)
{
    if (standard == "DDR3")
    {
        DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr3);
    }
    else if (standard == "DDR4")
    {
        DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr4);
    }
    else if (standard == "SALP-MASA")
    {
        SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, salp8);
    }
    else if (standard == "LPDDR3")
    {
        LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr3);
    }
    else if (standard == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr4);
    }
    else if (standard == "GDDR5")
    {
        GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, gddr5);
    }
    else if (standard == "HBM")
    {
        HBM* hbm = new HBM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, hbm);
    }
    else if (standard == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        WideIO* wio = new WideIO(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio);
    }
    else if (standard == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio2);
    }
    else if (standard == "STTMRAM")
    {
        STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, sttmram);
    }
    else if (standard == "PCM")
    {
        PCM* pcm = new PCM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, pcm);
    }
    // Various refresh mechanisms
    else if (standard == "DSARP")
    {
        DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, dsddr3_dsarp);
    }
    else if (standard == "ALDRAM")
    {
        ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, aldram);
    }
    else if (standard == "TLDRAM")
    {
        TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, tldram);
    }
}

template<typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, Config& configs2,
    Config& configs, T* spec)
{
    if (standard2 == "DDR3")
    {
        DDR3* ddr3 = new DDR3(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, ddr3);
    }
    else if (standard2 == "DDR4")
    {
        DDR4* ddr4 = new DDR4(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, ddr4);
    }
    else if (standard2 == "SALP-MASA")
    {
        SALP* salp8 = new SALP(configs2["org"], configs2["speed"], "SALP-MASA", configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, salp8);
    }
    else if (standard2 == "LPDDR3")
    {
        LPDDR3* lpddr3 = new LPDDR3(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, lpddr3);
    }
    else if (standard2 == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        LPDDR4* lpddr4 = new LPDDR4(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, lpddr4);
    }
    else if (standard2 == "GDDR5")
    {
        GDDR5* gddr5 = new GDDR5(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, gddr5);
    }
    else if (standard2 == "HBM")
    {
        HBM* hbm = new HBM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, hbm);
    }
    else if (standard2 == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        WideIO* wio = new WideIO(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, wio);
    }
    else if (standard2 == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        WideIO2* wio2 = new WideIO2(configs2["org"], configs2["speed"], configs2.get_channels());
        wio2->channel_width *= 2;
        start_run_simulation(configs, spec, configs2, wio2);
    }
    else if (standard2 == "STTMRAM")
    {
        STTMRAM* sttmram = new STTMRAM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, sttmram);
    }
    else if (standard2 == "PCM")
    {
        PCM* pcm = new PCM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, pcm);
    }
    // Various refresh mechanisms
    else if (standard2 == "DSARP")
    {
        DSARP* dsddr3_dsarp = new DSARP(configs2["org"], configs2["speed"], DSARP::Type::DSARP, configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, dsddr3_dsarp);
    }
    else if (standard2 == "ALDRAM")
    {
        ALDRAM* aldram = new ALDRAM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, aldram);
    }
    else if (standard2 == "TLDRAM")
    {
        TLDRAM* tldram = new TLDRAM(configs2["org"], configs2["speed"], configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, tldram);
    }
}

template<typename T, typename T2>
void start_run_simulation(const Config& configs, T* spec, const Config& configs2, T2* spec2)
{
    // initiate controller and memory for fast memory
    int C = configs.get_channels(), R = configs.get_ranks();
    // Check and Set channel, rank number for fast memory
    spec->set_channel_number(C);
    spec->set_rank_number(R);
    std::vector<Controller<T>*> ctrls;
    for (int c = 0; c < C; c++)
    {
        DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
        channel->id      = c;
        channel->regStats("");
        Controller<T>* ctrl = new Controller<T>(configs, channel);
        ctrls.push_back(ctrl);
    }
    Memory<T, Controller> memory(configs, ctrls);

    // initiate controller and memory for slow memory
    C = configs2.get_channels(), R = configs2.get_ranks();
    // Check and Set channel, rank number for slow memory
    spec2->set_channel_number(C);
    spec2->set_rank_number(R);
    std::vector<Controller<T2>*> ctrls2;
    for (int c = 0; c < C; c++)
    {
        DRAM<T2>* channel = new DRAM<T2>(spec2, T2::Level::Channel);
        channel->id       = c;
        channel->regStats("");
        Controller<T2>* ctrl = new Controller<T2>(configs2, channel);
        ctrls2.push_back(ctrl);
    }
    Memory<T2, Controller> memory2(configs2, ctrls2);

    if ((configs["trace_type"] == "DRAM") && (configs2["trace_type"] == "DRAM"))
    {
        simulation_run(configs, memory, configs2, memory2);
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }
}

template<typename T, typename T2>
void simulation_run(const Config& configs, Memory<T, Controller>& memory, const Config& configs2, Memory<T2, Controller>& memory2)
{
    VirtualMemory vmem(memory.max_address + memory2.max_address, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

    std::array<O3_CPU*, NUM_CPUS> ooo_cpu {};
    MEMORY_CONTROLLER<T, T2> memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, CPU_FREQUENCY / memory2.spec->speed_entry.freq, memory, memory2);
    CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
        LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
        L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
        L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
    PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
    CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
        STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
        L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
        ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
        DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
        CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
        BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
    CACHE cpu1_L2C("cpu1_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
        L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_L1D("cpu1_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
        L1D_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
    PageTableWalker cpu1_PTW("cpu1_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu1_L1D, vmem);
    CACHE cpu1_STLB("cpu1_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
        STLB_PREF_ACTIVATE_MASK, &cpu1_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_L1I("cpu1_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
        L1I_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_ITLB("cpu1_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
        ITLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_DTLB("cpu1_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
        DTLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    O3_CPU cpu1(CPU_1, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
        CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu1_ITLB, &cpu1_DTLB, &cpu1_L1I, &cpu1_L1D,
        BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

    // put cpu into the cpu array.
    ooo_cpu.at(0)            = &cpu0;
    ooo_cpu.at(NUM_CPUS - 1) = &cpu1;

    std::array<CACHE*, NUM_CACHES> caches {
        &LLC,
        &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
        &cpu1_L2C, &cpu1_L1D, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB};

    std::array<champsim::operable*, NUM_OPERABLES> operables {
        &cpu0, &cpu1,
        &LLC,
        &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
        &cpu1_L2C, &cpu1_L1D, &cpu1_PTW, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB,
        &memory_controller};

#else
    // put cpu into the cpu array.
    ooo_cpu.at(NUM_CPUS - 1) = &cpu0;

    std::array<CACHE*, NUM_CACHES> caches {&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB};

    std::array<champsim::operable*, NUM_OPERABLES> operables {&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &memory_controller};
#endif // CPU_USE_MULTIPLE_CORES

    // Output configuration details
    print_configuration_details(vmem);

    // SHARED CACHE
    for (O3_CPU* cpu : ooo_cpu)
    {
        cpu->initialize_core();
    }

    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    {
        (*it)->impl_prefetcher_initialize();
        (*it)->impl_replacement_initialize();
    }

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE) && (TEST_SWAPPING_UNIT == ENABLE)
    /* Test */
    all_warmup_complete = 1;
    while (true)
    {
        static uint64_t count = 0;
        static PACKET data1, data2;
        data1.cpu = data2.cpu = 0;
        data1.to_return = data2.to_return = {(MemoryRequestProducer*) (&LLC)};
        memory_controller.operate();

        // issue read and write
#define DATA1_TO_WRITE (200)
#define DATA2_TO_WRITE (201)
#define DATA1_ADDRESS  (0 << LOG2_BLOCK_SIZE)  // 2
#define DATA2_ADDRESS  (64 << LOG2_BLOCK_SIZE) // 66
        if (count == 100)                      // 150
        {
            int32_t status = 0;
            data1.address  = DATA1_ADDRESS;
            data2.address  = DATA2_ADDRESS;

            // read
            status         = memory_controller.add_rq(&data1);
            status         = memory_controller.add_rq(&data2);

            // write
            data1.data     = DATA1_TO_WRITE;
            data2.data     = DATA2_TO_WRITE;
            status         = memory_controller.add_wq(&data1);
            status         = memory_controller.add_wq(&data2);

            printf("status is %d\n", status);
        }

        if (count == 10'072)
        {
            int32_t status = 0;
            data1.address  = DATA1_ADDRESS;
            data2.address  = DATA2_ADDRESS;

            // read
            status         = memory_controller.add_rq(&data1);
            status         = memory_controller.add_rq(&data2);

            // write
            data1.data     = DATA1_TO_WRITE + 4;
            data2.data     = DATA2_TO_WRITE + 4;
            status         = memory_controller.add_wq(&data1);
            status         = memory_controller.add_wq(&data2);

            printf("status is %d\n", status);
        }

        if (count == 10'000)
        {
            memory_controller.start_swapping_segments(0 << LOG2_BLOCK_SIZE, 64 << LOG2_BLOCK_SIZE, 2);
        }

        if (count == 20'000)
        {
            memory_controller.start_swapping_segments(0 << LOG2_BLOCK_SIZE, 64 << LOG2_BLOCK_SIZE, 64);
        }

        count++;
        if (count >= warmup_instructions + simulation_instructions)
        {
            exit(0);
        }
    }
#endif // MEMORY_USE_SWAPPING_UNIT && TEST_SWAPPING_UNIT

    // simulation entry point
    while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>()))
    {
        uint64_t elapsed_second = (uint64_t) (time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
        elapsed_minute -= elapsed_hour * 60;
        elapsed_second -= (elapsed_hour * 3'600 + elapsed_minute * 60);

        for (auto op : operables)
        {
            try
            {
                op->_operate();
            }
            catch (champsim::deadlock& dl)
            {
                // ooo_cpu[dl.which]->print_deadlock();
                // std::cout << std::endl;
                // for (auto c : caches)
                for (auto c : operables)
                {
                    c->print_deadlock();
                    std::cout << std::endl;
                }

                abort();
            }
        }
        // we don't need to sort here since the clock difference of cpu and memory is expressed in the memory controller.
        // std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

        for (std::size_t i = 0; i < ooo_cpu.size(); ++i)
        {
            // read from trace
            while (ooo_cpu[i]->fetch_stall == 0 && ooo_cpu[i]->instrs_to_read_this_cycle > 0)
            {
                ooo_cpu[i]->init_instruction(traces[i]->get());
            }

            // heartbeat information
            if (show_heartbeat && (ooo_cpu[i]->num_retired >= ooo_cpu[i]->next_print_instruction))
            {
                float cumulative_ipc;
                if (warmup_complete[i])
                    cumulative_ipc = (1.0 * (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
                else
                    cumulative_ipc = (1.0 * ooo_cpu[i]->num_retired) / ooo_cpu[i]->current_cycle;
                float heartbeat_ipc = (1.0 * ooo_cpu[i]->num_retired - ooo_cpu[i]->last_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->last_sim_cycle);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
                // fprintf(output_statistics.file_handler, "Heartbeat CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
                // fprintf(output_statistics.file_handler, " heartbeat IPC: %f cumulative IPC: %f", heartbeat_ipc, cumulative_ipc);
                // fprintf(output_statistics.file_handler, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
                std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
                std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
                std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#else
                std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
                std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
                std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif // PRINT_STATISTICS_INTO_FILE

                ooo_cpu[i]->next_print_instruction += STAT_PRINTING_PERIOD;

                ooo_cpu[i]->last_sim_instr = ooo_cpu[i]->num_retired;
                ooo_cpu[i]->last_sim_cycle = ooo_cpu[i]->current_cycle;
            }

            // check for warmup
            // warmup complete
            if ((warmup_complete[i] == 0) && (ooo_cpu[i]->num_retired > warmup_instructions))
            {
                warmup_complete[i] = 1;
                all_warmup_complete++;
            }
            if (all_warmup_complete == NUM_CPUS)
            { // this part is called only once
                // when all cores are warmed up
                all_warmup_complete++;
                finish_warmup(ooo_cpu, caches);
            }

            // simulation complete
            if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0) && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions)))
            {
                simulation_complete[i]       = 1;
                ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
                ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
                fprintf(output_statistics.file_handler, "Finished CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
                fprintf(output_statistics.file_handler, " cumulative IPC: %f", ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
                fprintf(output_statistics.file_handler, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
#else
                std::cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
                std::cout << " cumulative IPC: " << ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
                std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif // PRINT_STATISTICS_INTO_FILE

                for (auto it = caches.rbegin(); it != caches.rend(); ++it)
                    record_roi_stats(i, *it);
            }
        }
    }

    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    memory2.finish();
    Stats::statlist.printall();

    uint64_t elapsed_second = (uint64_t) (time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour * 60;
    elapsed_second -= (elapsed_hour * 3'600 + elapsed_minute * 60);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(output_statistics.file_handler, "\nChampSim completed all CPUs\n");
    if (NUM_CPUS > 1)
    {
        fprintf(output_statistics.file_handler, "\nTotal Simulation Statistics (not including warmup)\n");
        for (uint32_t i = 0; i < NUM_CPUS; i++)
        {
            fprintf(output_statistics.file_handler, "\nCPU %d cumulative IPC: %f", i, (float) (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle));
            fprintf(output_statistics.file_handler, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr, ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
            for (auto it = caches.rbegin(); it != caches.rend(); ++it)
                print_sim_stats(i, *it);
        }
    }

    fprintf(output_statistics.file_handler, "\nRegion of Interest Statistics\n");
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
        fprintf(output_statistics.file_handler, "\nCPU %d cumulative IPC: %f", i, ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
        fprintf(output_statistics.file_handler, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
            print_roi_stats(i, *it);
    }
#else
    std::cout << std::endl
              << "ChampSim completed all CPUs" << std::endl;
    if (NUM_CPUS > 1)
    {
        std::cout << std::endl
                  << "Total Simulation Statistics (not including warmup)" << std::endl;
        for (uint32_t i = 0; i < NUM_CPUS; i++)
        {
            std::cout << std::endl
                      << "CPU " << i
                      << " cumulative IPC: " << (float) (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
            std::cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
                      << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << std::endl;
            for (auto it = caches.rbegin(); it != caches.rend(); ++it)
                print_sim_stats(i, *it);
        }
    }

    std::cout << std::endl
              << "Region of Interest Statistics" << std::endl;
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
        std::cout << std::endl
                  << "CPU " << i << " cumulative IPC: " << ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
        std::cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << std::endl;
        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
            print_roi_stats(i, *it);
    }
#endif // PRINT_STATISTICS_INTO_FILE

    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        (*it)->impl_prefetcher_final_stats();

    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        (*it)->impl_replacement_final_stats();

#ifndef CRC2_COMPILE
    print_branch_stats(ooo_cpu);
#endif // CRC2_COMPILE
}

#else

void configure_memory_to_run_simulation(const std::string& standard, Config& configs, simulator_input_parameter& input_parameter)
{
    if (standard == "DDR3")
    {
        DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
        start_run_simulation(configs, ddr3, input_parameter);
    }
    else if (standard == "DDR4")
    {
        DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
        start_run_simulation(configs, ddr4, input_parameter);
    }
    else if (standard == "SALP-MASA")
    {
        SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        start_run_simulation(configs, salp8, input_parameter);
    }
    else if (standard == "LPDDR3")
    {
        LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
        start_run_simulation(configs, lpddr3, input_parameter);
    }
    else if (standard == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
        start_run_simulation(configs, lpddr4, input_parameter);
    }
    else if (standard == "GDDR5")
    {
        GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
        start_run_simulation(configs, gddr5, input_parameter);
    }
    else if (standard == "HBM")
    {
        HBM* hbm = new HBM(configs["org"], configs["speed"]);
        start_run_simulation(configs, hbm, input_parameter);
    }
    else if (standard == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        WideIO* wio = new WideIO(configs["org"], configs["speed"]);
        start_run_simulation(configs, wio, input_parameter);
    }
    else if (standard == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        start_run_simulation(configs, wio2, input_parameter);
    }
    else if (standard == "STTMRAM")
    {
        STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
        start_run_simulation(configs, sttmram, input_parameter);
    }
    else if (standard == "PCM")
    {
        PCM* pcm = new PCM(configs["org"], configs["speed"]);
        start_run_simulation(configs, pcm, input_parameter);
    }
    // Various refresh mechanisms
    else if (standard == "DSARP")
    {
        DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
        start_run_simulation(configs, dsddr3_dsarp, input_parameter);
    }
    else if (standard == "ALDRAM")
    {
        ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
        start_run_simulation(configs, aldram, input_parameter);
    }
    else if (standard == "TLDRAM")
    {
        TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        start_run_simulation(configs, tldram, input_parameter);
    }
}

template<typename T>
void start_run_simulation(const Config& configs, T* spec, simulator_input_parameter& input_parameter)
{
    // Initiate controller and memory for fast memory
    int C = configs.get_channels(), R = configs.get_ranks();
    // Check and Set channel, rank number for fast memory
    spec->set_channel_number(C);
    spec->set_rank_number(R);
    std::vector<Controller<T>*> ctrls;
    for (int c = 0; c < C; c++)
    {
        DRAM<T>* channel = new DRAM<T>(spec, T::Level::Channel);
        channel->id      = c;
        channel->regStats("");
        Controller<T>* ctrl = new Controller<T>(configs, channel);
        ctrls.push_back(ctrl);
    }
    Memory<T, Controller> memory(configs, ctrls);

    if (configs["trace_type"] == "DRAM")
    {
        simulation_run(configs, memory, input_parameter);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }
}

template<typename T>
void simulation_run(const Config& configs, ramulator::Memory<T, ramulator::Controller>& memory, simulator_input_parameter& input_parameter)
{
    /** @note Prepare the hardware modules */
    champsim::configured::generated_environment<T> gen_environment(memory);

    if (input_parameter.hide_given)
    {
        for (O3_CPU& cpu : gen_environment.cpu_view())
            cpu.show_heartbeat = false;
    }

    fmt::print("\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: {}\nSimulation Instructions: {}\nNumber of CPUs: {}\nPage size: {}\n\n", phases.at(0).length, phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: %ld\nSimulation Instructions: %ld\nNumber of CPUs: %ld\nPage size: %d\n\n", phases.at(0).length, phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#endif // PRINT_STATISTICS_INTO_FILE

    auto phase_stats = champsim::main(gen_environment, input_parameter.phases, input_parameter.traces);

    fmt::print("\nChampSim completed all CPUs\n\n");
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nChampSim completed all CPUs\n\n");
#endif // PRINT_STATISTICS_INTO_FILE

    // VirtualMemory vmem(memory.max_address, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

    // std::array<O3_CPU*, NUM_CPUS> ooo_cpu {};
    // MEMORY_CONTROLLER<T> memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, memory);
    // CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
    //     LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
    //     L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
    //     L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
    // CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
    //     STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
    //     L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
    //     ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
    //     DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    // O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
    //     CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
    //     BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
    CACHE cpu1_L2C("cpu1_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
        L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_L1D("cpu1_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
        L1D_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
    PageTableWalker cpu1_PTW("cpu1_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu1_L1D, vmem);
    CACHE cpu1_STLB("cpu1_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
        STLB_PREF_ACTIVATE_MASK, &cpu1_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_L1I("cpu1_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
        L1I_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_ITLB("cpu1_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
        ITLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    CACHE cpu1_DTLB("cpu1_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
        DTLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
    O3_CPU cpu1(CPU_1, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
        CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu1_ITLB, &cpu1_DTLB, &cpu1_L1I, &cpu1_L1D,
        BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

    // put cpu into the cpu array.
    ooo_cpu.at(0)            = &cpu0;
    ooo_cpu.at(NUM_CPUS - 1) = &cpu1;

    std::array<CACHE*, NUM_CACHES> caches {
        &LLC,
        &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
        &cpu1_L2C, &cpu1_L1D, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB};

    std::array<champsim::operable*, NUM_OPERABLES> operables {
        &cpu0, &cpu1,
        &LLC,
        &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
        &cpu1_L2C, &cpu1_L1D, &cpu1_PTW, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB,
        &memory_controller};
#endif // CPU_USE_MULTIPLE_CORES

    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    Stats::statlist.printall();

    champsim::plain_printer {std::cout}.print(phase_stats);

    for (CACHE& cache : gen_environment.cache_view())
        cache.impl_prefetcher_final_stats();

    for (CACHE& cache : gen_environment.cache_view())
        cache.impl_replacement_final_stats();

    if (input_parameter.json_given)
    {
        std::ofstream json_file {input_parameter.json_file_name};
        champsim::json_printer {json_file}.print(phase_stats);
    }
}
#endif // MEMORY_USE_HYBRID

#else

// void simulation_run()
// {
//     VirtualMemory vmem(MEMORY_CAPACITY, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

//     std::array<O3_CPU*, NUM_CPUS> ooo_cpu {};
//     MEMORY_CONTROLLER memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE);
//     CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
//         LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
//         L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
//         L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
//     CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
//         STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
//         L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
//         ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
//         DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
//         CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
//         BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

// #if (CPU_USE_MULTIPLE_CORES == ENABLE)
//     CACHE cpu1_L2C("cpu1_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
//         L2C_PREF_ACTIVATE_MASK, &LLC, CPU_L2C_PREFETCHER, CPU_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu1_L1D("cpu1_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
//         L1D_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1D_PREFETCHER, CPU_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     PageTableWalker cpu1_PTW("cpu1_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu1_L1D, vmem);
//     CACHE cpu1_STLB("cpu1_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
//         STLB_PREF_ACTIVATE_MASK, &cpu1_PTW, CPU_STLB_PREFETCHER, CPU_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu1_L1I("cpu1_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
//         L1I_PREF_ACTIVATE_MASK, &cpu1_L2C, CPU_L1I_PREFETCHER, CPU_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu1_ITLB("cpu1_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
//         ITLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_ITLB_PREFETCHER, CPU_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     CACHE cpu1_DTLB("cpu1_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
//         DTLB_PREF_ACTIVATE_MASK, &cpu1_STLB, CPU_DTLB_PREFETCHER, CPU_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
//     O3_CPU cpu1(CPU_1, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
//         CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu1_ITLB, &cpu1_DTLB, &cpu1_L1I, &cpu1_L1D,
//         BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

//     // put cpu into the cpu array.
//     ooo_cpu.at(0)            = &cpu0;
//     ooo_cpu.at(NUM_CPUS - 1) = &cpu1;

//     std::array<CACHE*, NUM_CACHES> caches {
//         &LLC,
//         &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
//         &cpu1_L2C, &cpu1_L1D, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB};

//     std::array<champsim::operable*, NUM_OPERABLES> operables {
//         &cpu0, &cpu1,
//         &LLC,
//         &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB,
//         &cpu1_L2C, &cpu1_L1D, &cpu1_PTW, &cpu1_STLB, &cpu1_L1I, &cpu1_ITLB, &cpu1_DTLB,
//         &memory_controller};

// #else
//     // put cpu into the cpu array.
//     ooo_cpu.at(NUM_CPUS - 1) = &cpu0;

//     std::array<CACHE*, NUM_CACHES> caches {&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB};

//     std::array<champsim::operable*, NUM_OPERABLES> operables {&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &memory_controller};
// #endif // CPU_USE_MULTIPLE_CORES

//     // Output configuration details
//     print_configuration_details(vmem);

//     // SHARED CACHE
//     for (O3_CPU* cpu : ooo_cpu)
//     {
//         cpu->initialize_core();
//     }

//     for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//     {
//         (*it)->impl_prefetcher_initialize();
//         (*it)->impl_replacement_initialize();
//     }

//     // simulation entry point
//     while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>()))
//     {
//         uint64_t elapsed_second = (uint64_t) (time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
//         elapsed_minute -= elapsed_hour * 60;
//         elapsed_second -= (elapsed_hour * 3'600 + elapsed_minute * 60);

//         for (auto op : operables)
//         {
//             try
//             {
//                 op->_operate();
//             }
//             catch (champsim::deadlock& dl)
//             {
//                 // ooo_cpu[dl.which]->print_deadlock();
//                 // std::cout << std::endl;
//                 // for (auto c : caches)
//                 for (auto c : operables)
//                 {
//                     c->print_deadlock();
//                     std::cout << std::endl;
//                 }

//                 abort();
//             }
//         }
//         std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

//         for (std::size_t i = 0; i < ooo_cpu.size(); ++i)
//         {
//             // read from trace
//             while (ooo_cpu[i]->fetch_stall == 0 && ooo_cpu[i]->instrs_to_read_this_cycle > 0)
//             {
//                 ooo_cpu[i]->init_instruction(traces[i]->get());
//             }

//             // heartbeat information
//             if (show_heartbeat && (ooo_cpu[i]->num_retired >= ooo_cpu[i]->next_print_instruction))
//             {
//                 float cumulative_ipc;
//                 if (warmup_complete[i])
//                     cumulative_ipc = (1.0 * (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
//                 else
//                     cumulative_ipc = (1.0 * ooo_cpu[i]->num_retired) / ooo_cpu[i]->current_cycle;
//                 float heartbeat_ipc = (1.0 * ooo_cpu[i]->num_retired - ooo_cpu[i]->last_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->last_sim_cycle);

// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//                 // fprintf(output_statistics.file_handler, "Heartbeat CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
//                 // fprintf(output_statistics.file_handler, " heartbeat IPC: %f cumulative IPC: %f", heartbeat_ipc, cumulative_ipc);
//                 // fprintf(output_statistics.file_handler, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
//                 std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
//                 std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
//                 std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
// #else
//                 std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
//                 std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
//                 std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
// #endif

//                 ooo_cpu[i]->next_print_instruction += STAT_PRINTING_PERIOD;

//                 ooo_cpu[i]->last_sim_instr = ooo_cpu[i]->num_retired;
//                 ooo_cpu[i]->last_sim_cycle = ooo_cpu[i]->current_cycle;
//             }

//             // check for warmup
//             // warmup complete
//             if ((warmup_complete[i] == 0) && (ooo_cpu[i]->num_retired > warmup_instructions))
//             {
//                 warmup_complete[i] = 1;
//                 all_warmup_complete++;
//             }
//             if (all_warmup_complete == NUM_CPUS)
//             { // this part is called only once
//                 // when all cores are warmed up
//                 all_warmup_complete++;
//                 finish_warmup(ooo_cpu, caches, memory_controller);
//             }

//             // simulation complete
//             if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0) && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions)))
//             {
//                 simulation_complete[i]       = 1;
//                 ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
//                 ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//                 fprintf(output_statistics.file_handler, "Finished CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
//                 fprintf(output_statistics.file_handler, " cumulative IPC: %f", ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
//                 fprintf(output_statistics.file_handler, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
// #else
//                 std::cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
//                 std::cout << " cumulative IPC: " << ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
//                 std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
// #endif

//                 for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//                     record_roi_stats(i, *it);
//             }
//         }
//     }

//     uint64_t elapsed_second = (uint64_t) (time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
//     elapsed_minute -= elapsed_hour * 60;
//     elapsed_second -= (elapsed_hour * 3'600 + elapsed_minute * 60);

// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//     fprintf(output_statistics.file_handler, "\nChampSim completed all CPUs\n");
//     if (NUM_CPUS > 1)
//     {
//         fprintf(output_statistics.file_handler, "\nTotal Simulation Statistics (not including warmup)\n");
//         for (uint32_t i = 0; i < NUM_CPUS; i++)
//         {
//             fprintf(output_statistics.file_handler, "\nCPU %d cumulative IPC: %f", i, (float) (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle));
//             fprintf(output_statistics.file_handler, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr, ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
//             for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//                 print_sim_stats(i, *it);
//         }
//     }

//     fprintf(output_statistics.file_handler, "\nRegion of Interest Statistics\n");
//     for (uint32_t i = 0; i < NUM_CPUS; i++)
//     {
//         fprintf(output_statistics.file_handler, "\nCPU %d cumulative IPC: %f", i, ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
//         fprintf(output_statistics.file_handler, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
//         for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//             print_roi_stats(i, *it);
//     }
// #else
//     std::cout << std::endl
//               << "ChampSim completed all CPUs" << std::endl;
//     if (NUM_CPUS > 1)
//     {
//         std::cout << std::endl
//                   << "Total Simulation Statistics (not including warmup)" << std::endl;
//         for (uint32_t i = 0; i < NUM_CPUS; i++)
//         {
//             std::cout << std::endl
//                       << "CPU " << i
//                       << " cumulative IPC: " << (float) (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
//             std::cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
//                       << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << std::endl;
//             for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//                 print_sim_stats(i, *it);
//         }
//     }

//     std::cout << std::endl
//               << "Region of Interest Statistics" << std::endl;
//     for (uint32_t i = 0; i < NUM_CPUS; i++)
//     {
//         std::cout << std::endl
//                   << "CPU " << i << " cumulative IPC: " << ((float) ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
//         std::cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << std::endl;
//         for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//             print_roi_stats(i, *it);
//     }
// #endif // PRINT_STATISTICS_INTO_FILE

//     for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//         (*it)->impl_prefetcher_final_stats();

//     for (auto it = caches.rbegin(); it != caches.rend(); ++it)
//         (*it)->impl_replacement_final_stats();

// #ifndef CRC2_COMPILE
// #if (USER_CODES == ENABLE)
//     print_memory_stats(memory_controller);
// #else
//     print_dram_stats();
// #endif

//     print_branch_stats(ooo_cpu);
// #endif // CRC2_COMPILE
// }

#endif // RAMULATOR

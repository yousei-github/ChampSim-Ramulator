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

#if (RAMULATOR == ENABLE)

#if (MEMORY_USE_HYBRID == ENABLE)

void configure_fast_memory_to_run_simulation(const std::string& standard, ramulator::Config& configs, const std::string& standard2, ramulator::Config& configs2, simulator_input_parameter& input_parameter);

template<typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, ramulator::Config& configs2, ramulator::Config& configs, T* spec, simulator_input_parameter& input_parameter);

template<typename T, typename T2>
void start_run_simulation(const ramulator::Config& configs, T* spec, const ramulator::Config& configs2, T2* spec2, simulator_input_parameter& input_parameter);

template<typename T, typename T2>
void simulation_run(const ramulator::Config& configs, ramulator::Memory<T, ramulator::Controller>& memory, const ramulator::Config& configs2, ramulator::Memory<T2, ramulator::Controller>& memory2, simulator_input_parameter& input_parameter);

#else

void configure_memory_to_run_simulation(const std::string& standard, ramulator::Config& configs, simulator_input_parameter& input_parameter);

template<typename T>
void start_run_simulation(const ramulator::Config& configs, T* spec, simulator_input_parameter& input_parameter);

template<typename T>
void simulation_run(const ramulator::Config& configs, ramulator::Memory<T, ramulator::Controller>& memory, simulator_input_parameter& input_parameter);

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
        // Show how many cores your computer have
        std::printf("(%s) Thread %d of %d threads says hello.\n", __func__, omp_get_thread_num(), omp_get_num_threads());
    }
#endif // USE_OPENMP

    simulator_input_parameter input_parameter;

    /** @note Process the input parameters */
    if (argc < 2)
    {
#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
        std::printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> [--stats <filename>] <configs-file> <configs-file2> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 ramulator-configs1.cfg ramulator-configs2.cfg cpu_trace.xz\n",
            argv[0], argv[0]);
#else
        std::printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> [--stats <filename>] <configs-file> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 ramulator-configs1.cfg cpu_trace.xz\n",
            argv[0], argv[0]);
#endif // MEMORY_USE_HYBRID
#else
        std::printf(
            "Usage: %s --warmup-instructions <warmup-instructions> --simulation-instructions <simulation-instructions> <trace-filename1>\n"
            "Example: %s --warmup-instructions 1000000 --simulation-instructions 2000000 cpu_trace.xz\n",
            argv[0], argv[0]);
#endif // RAMULATOR

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
        std::printf("\nNote: assign cpu traces to each cpu by appending multiple trace files to the command line.\n");
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

    std::transform(std::begin(input_parameter.trace_names), std::end(input_parameter.trace_names), std::back_inserter(input_parameter.traces),
        [knob_cloudsuite = input_parameter.knob_cloudsuite, repeat = input_parameter.simulation_given, i = uint8_t(0)](auto name) mutable
        { return get_tracereader(name, i++, knob_cloudsuite, repeat); });

    input_parameter.phases.push_back(champsim::phase_info {"Warmup", true, input_parameter.warmup_instructions, std::vector<std::size_t>(std::size(input_parameter.trace_names), 0), input_parameter.trace_names});          // Push back warmup phase
    input_parameter.phases.push_back(champsim::phase_info {"Simulation", false, input_parameter.simulation_instructions, std::vector<std::size_t>(std::size(input_parameter.trace_names), 0), input_parameter.trace_names}); // Push back simulation phase

    for (auto& p : input_parameter.phases)
    {
        std::iota(std::begin(p.trace_index), std::end(p.trace_index), 0);
    }

    /** @note Prepare the Ramulator framework */
#if (RAMULATOR == ENABLE)
    // Prepare initialization for Ramulator.
#if (MEMORY_USE_HYBRID == ENABLE)
    // Read memory configuration file.
    ramulator::Config configs(argv[start_position_of_configs]);      // Fast memory
    ramulator::Config configs2(argv[start_position_of_configs + 1]); // Slow memory

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
    ramulator::Config configs(argv[start_position_of_configs]);

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
    configure_fast_memory_to_run_simulation(standard, configs, standard2, configs2, input_parameter);
#else
    configure_memory_to_run_simulation(standard, configs, input_parameter);
#endif // MEMORY_USE_HYBRID

    std::printf("Simulation done. Statistics written to %s\n", stats_out.c_str());
#else
    //simulation_run();
    std::printf("Simulation done.\n");
#endif // RAMULATOR

    return EXIT_SUCCESS;
}

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)

void configure_fast_memory_to_run_simulation(const std::string& standard, ramulator::Config& configs, const std::string& standard2, ramulator::Config& configs2, simulator_input_parameter& input_parameter)
{
    if (standard == "DDR3")
    {
        ramulator::DDR3* ddr3 = new ramulator::DDR3(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr3, input_parameter);
    }
    else if (standard == "DDR4")
    {
        ramulator::DDR4* ddr4 = new ramulator::DDR4(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, ddr4, input_parameter);
    }
    else if (standard == "SALP-MASA")
    {
        ramulator::SALP* salp8 = new ramulator::SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, salp8, input_parameter);
    }
    else if (standard == "LPDDR3")
    {
        ramulator::LPDDR3* lpddr3 = new ramulator::LPDDR3(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr3, input_parameter);
    }
    else if (standard == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::LPDDR4* lpddr4 = new ramulator::LPDDR4(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, lpddr4, input_parameter);
    }
    else if (standard == "GDDR5")
    {
        ramulator::GDDR5* gddr5 = new ramulator::GDDR5(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, gddr5, input_parameter);
    }
    else if (standard == "HBM")
    {
        ramulator::HBM* hbm = new ramulator::HBM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, hbm, input_parameter);
    }
    else if (standard == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        ramulator::WideIO* wio = new ramulator::WideIO(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio, input_parameter);
    }
    else if (standard == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::WideIO2* wio2 = new ramulator::WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, wio2, input_parameter);
    }
    else if (standard == "STTMRAM")
    {
        ramulator::STTMRAM* sttmram = new ramulator::STTMRAM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, sttmram, input_parameter);
    }
    else if (standard == "PCM")
    {
        ramulator::PCM* pcm = new ramulator::PCM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, pcm, input_parameter);
    }
    // Various refresh mechanisms
    else if (standard == "DSARP")
    {
        ramulator::DSARP* dsddr3_dsarp = new ramulator::DSARP(configs["org"], configs["speed"], ramulator::DSARP::Type::DSARP, configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, dsddr3_dsarp, input_parameter);
    }
    else if (standard == "ALDRAM")
    {
        ramulator::ALDRAM* aldram = new ramulator::ALDRAM(configs["org"], configs["speed"]);
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, aldram, input_parameter);
    }
    else if (standard == "TLDRAM")
    {
        ramulator::TLDRAM* tldram = new ramulator::TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        next_configure_slow_memory_to_run_simulation(standard2, configs2, configs, tldram, input_parameter);
    }
}

template<typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, ramulator::Config& configs2, ramulator::Config& configs, T* spec, simulator_input_parameter& input_parameter)
{
    if (standard2 == "DDR3")
    {
        ramulator::DDR3* ddr3 = new ramulator::DDR3(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, ddr3, input_parameter);
    }
    else if (standard2 == "DDR4")
    {
        ramulator::DDR4* ddr4 = new ramulator::DDR4(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, ddr4, input_parameter);
    }
    else if (standard2 == "SALP-MASA")
    {
        ramulator::SALP* salp8 = new ramulator::SALP(configs2["org"], configs2["speed"], "SALP-MASA", configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, salp8, input_parameter);
    }
    else if (standard2 == "LPDDR3")
    {
        ramulator::LPDDR3* lpddr3 = new ramulator::LPDDR3(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, lpddr3, input_parameter);
    }
    else if (standard2 == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::LPDDR4* lpddr4 = new ramulator::LPDDR4(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, lpddr4, input_parameter);
    }
    else if (standard2 == "GDDR5")
    {
        ramulator::GDDR5* gddr5 = new ramulator::GDDR5(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, gddr5, input_parameter);
    }
    else if (standard2 == "HBM")
    {
        ramulator::HBM* hbm = new ramulator::HBM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, hbm, input_parameter);
    }
    else if (standard2 == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        ramulator::WideIO* wio = new ramulator::WideIO(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, wio, input_parameter);
    }
    else if (standard2 == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::WideIO2* wio2 = new ramulator::WideIO2(configs2["org"], configs2["speed"], configs2.get_channels());
        wio2->channel_width *= 2;
        start_run_simulation(configs, spec, configs2, wio2, input_parameter);
    }
    else if (standard2 == "STTMRAM")
    {
        ramulator::STTMRAM* sttmram = new ramulator::STTMRAM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, sttmram, input_parameter);
    }
    else if (standard2 == "PCM")
    {
        ramulator::PCM* pcm = new ramulator::PCM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, pcm, input_parameter);
    }
    // Various refresh mechanisms
    else if (standard2 == "DSARP")
    {
        ramulator::DSARP* dsddr3_dsarp = new ramulator::DSARP(configs2["org"], configs2["speed"], ramulator::DSARP::Type::DSARP, configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, dsddr3_dsarp, input_parameter);
    }
    else if (standard2 == "ALDRAM")
    {
        ramulator::ALDRAM* aldram = new ramulator::ALDRAM(configs2["org"], configs2["speed"]);
        start_run_simulation(configs, spec, configs2, aldram, input_parameter);
    }
    else if (standard2 == "TLDRAM")
    {
        ramulator::TLDRAM* tldram = new ramulator::TLDRAM(configs2["org"], configs2["speed"], configs2.get_subarrays());
        start_run_simulation(configs, spec, configs2, tldram, input_parameter);
    }
}

template<typename T, typename T2>
void start_run_simulation(const ramulator::Config& configs, T* spec, const ramulator::Config& configs2, T2* spec2, simulator_input_parameter& input_parameter)
{
    // Initiate controller and memory for fast memory
    int C = configs.get_channels(), R = configs.get_ranks();
    // Check and Set channel, rank number for fast memory
    spec->set_channel_number(C);
    spec->set_rank_number(R);
    std::vector<ramulator::Controller<T>*> ctrls;
    for (int c = 0; c < C; c++)
    {
        ramulator::DRAM<T>* channel = new ramulator::DRAM<T>(spec, T::Level::Channel);
        channel->id                 = c;
        channel->regStats("");
        ramulator::Controller<T>* ctrl = new ramulator::Controller<T>(configs, channel);
        ctrls.push_back(ctrl);
    }
    ramulator::Memory<T, ramulator::Controller> memory(configs, ctrls);

    // Initiate controller and memory for slow memory
    C = configs2.get_channels(), R = configs2.get_ranks();
    // Check and Set channel, rank number for slow memory
    spec2->set_channel_number(C);
    spec2->set_rank_number(R);
    std::vector<ramulator::Controller<T2>*> ctrls2;
    for (int c = 0; c < C; c++)
    {
        ramulator::DRAM<T2>* channel = new ramulator::DRAM<T2>(spec2, T2::Level::Channel);
        channel->id                  = c;
        channel->regStats("");
        ramulator::Controller<T2>* ctrl = new ramulator::Controller<T2>(configs2, channel);
        ctrls2.push_back(ctrl);
    }
    ramulator::Memory<T2, ramulator::Controller> memory2(configs2, ctrls2);

    if ((configs["trace_type"] == "DRAM") && (configs2["trace_type"] == "DRAM"))
    {
        simulation_run(configs, memory, configs2, memory2, input_parameter);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }
}

template<typename T, typename T2>
void simulation_run(const ramulator::Config& configs, ramulator::Memory<T, ramulator::Controller>& memory, const ramulator::Config& configs2, ramulator::Memory<T2, ramulator::Controller>& memory2, simulator_input_parameter& input_parameter)
{
    /** @note Prepare the hardware modules */
    champsim::configured::generated_environment<T, T2> gen_environment(memory, memory2);

    if (input_parameter.hide_given)
    {
        for (O3_CPU& cpu : gen_environment.cpu_view())
            cpu.show_heartbeat = false;
    }

#if (USE_VCPKG == ENABLE)
    fmt::print("\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: {}\nSimulation Instructions: {}\nNumber of CPUs: {}\nPage size: {}\n\n", input_parameter.phases.at(0).length, input_parameter.phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: %ld\nSimulation Instructions: %ld\nNumber of CPUs: %ld\nPage size: %d\n\n", input_parameter.phases.at(0).length, input_parameter.phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#endif // PRINT_STATISTICS_INTO_FILE

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

    auto phase_stats = champsim::main(gen_environment, input_parameter.phases, input_parameter.traces);

#if (USE_VCPKG == ENABLE)
    fmt::print("\nChampSim completed all CPUs\n\n");
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nChampSim completed all CPUs\n\n");
#endif // PRINT_STATISTICS_INTO_FILE

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
    /** @todo Waiting for updating*/

#endif // CPU_USE_MULTIPLE_CORES

    // This a workaround for statistics set only initially lost in the end
    memory.finish();
    memory2.finish();
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

#else

void configure_memory_to_run_simulation(const std::string& standard, ramulator::Config& configs, simulator_input_parameter& input_parameter)
{
    /** @todo When to delete the memory? */
    if (standard == "DDR3")
    {
        ramulator::DDR3* ddr3 = new ramulator::DDR3(configs["org"], configs["speed"]);
        start_run_simulation(configs, ddr3, input_parameter);
    }
    else if (standard == "DDR4")
    {
        ramulator::DDR4* ddr4 = new ramulator::DDR4(configs["org"], configs["speed"]);
        start_run_simulation(configs, ddr4, input_parameter);
    }
    else if (standard == "SALP-MASA")
    {
        ramulator::SALP* salp8 = new ramulator::SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
        start_run_simulation(configs, salp8, input_parameter);
    }
    else if (standard == "LPDDR3")
    {
        ramulator::LPDDR3* lpddr3 = new ramulator::LPDDR3(configs["org"], configs["speed"]);
        start_run_simulation(configs, lpddr3, input_parameter);
    }
    else if (standard == "LPDDR4")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::LPDDR4* lpddr4 = new ramulator::LPDDR4(configs["org"], configs["speed"]);
        start_run_simulation(configs, lpddr4, input_parameter);
    }
    else if (standard == "GDDR5")
    {
        ramulator::GDDR5* gddr5 = new ramulator::GDDR5(configs["org"], configs["speed"]);
        start_run_simulation(configs, gddr5, input_parameter);
    }
    else if (standard == "HBM")
    {
        ramulator::HBM* hbm = new ramulator::HBM(configs["org"], configs["speed"]);
        start_run_simulation(configs, hbm, input_parameter);
    }
    else if (standard == "WideIO")
    {
        // total cap: 1GB, 1/4 of others
        ramulator::WideIO* wio = new ramulator::WideIO(configs["org"], configs["speed"]);
        start_run_simulation(configs, wio, input_parameter);
    }
    else if (standard == "WideIO2")
    {
        // total cap: 2GB, 1/2 of others
        ramulator::WideIO2* wio2 = new ramulator::WideIO2(configs["org"], configs["speed"], configs.get_channels());
        wio2->channel_width *= 2;
        start_run_simulation(configs, wio2, input_parameter);
    }
    else if (standard == "STTMRAM")
    {
        ramulator::STTMRAM* sttmram = new ramulator::STTMRAM(configs["org"], configs["speed"]);
        start_run_simulation(configs, sttmram, input_parameter);
    }
    else if (standard == "PCM")
    {
        ramulator::PCM* pcm = new ramulator::PCM(configs["org"], configs["speed"]);
        start_run_simulation(configs, pcm, input_parameter);
    }
    // Various refresh mechanisms
    else if (standard == "DSARP")
    {
        ramulator::DSARP* dsddr3_dsarp = new ramulator::DSARP(configs["org"], configs["speed"], ramulator::DSARP::Type::DSARP, configs.get_subarrays());
        start_run_simulation(configs, dsddr3_dsarp, input_parameter);
    }
    else if (standard == "ALDRAM")
    {
        ramulator::ALDRAM* aldram = new ramulator::ALDRAM(configs["org"], configs["speed"]);
        start_run_simulation(configs, aldram, input_parameter);
    }
    else if (standard == "TLDRAM")
    {
        ramulator::TLDRAM* tldram = new ramulator::TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
        start_run_simulation(configs, tldram, input_parameter);
    }
}

template<typename T>
void start_run_simulation(const ramulator::Config& configs, T* spec, simulator_input_parameter& input_parameter)
{
    // Initiate controller and memory for fast memory
    int C = configs.get_channels(), R = configs.get_ranks();
    // Check and Set channel, rank number for fast memory
    spec->set_channel_number(C);
    spec->set_rank_number(R);
    std::vector<ramulator::Controller<T>*> ctrls;
    for (int c = 0; c < C; c++)
    {
        ramulator::DRAM<T>* channel = new ramulator::DRAM<T>(spec, T::Level::Channel);
        channel->id = c;
        channel->regStats("");
        ramulator::Controller<T>* ctrl = new ramulator::Controller<T>(configs, channel);
        ctrls.push_back(ctrl);
    }
    ramulator::Memory<T, ramulator::Controller> memory(configs, ctrls);

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
void simulation_run(const ramulator::Config& configs, ramulator::Memory<T, ramulator::Controller>& memory, simulator_input_parameter& input_parameter)
{
    /** @note Prepare the hardware modules */
    champsim::configured::generated_environment<T> gen_environment(memory);

    if (input_parameter.hide_given)
    {
        for (O3_CPU& cpu : gen_environment.cpu_view())
            cpu.show_heartbeat = false;
    }

#if (USE_VCPKG == ENABLE)
    fmt::print("\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: {}\nSimulation Instructions: {}\nNumber of CPUs: {}\nPage size: {}\n\n", input_parameter.phases.at(0).length, input_parameter.phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\n*** ChampSim Multicore Out-of-Order Simulator ***\nWarmup Instructions: %ld\nSimulation Instructions: %ld\nNumber of CPUs: %ld\nPage size: %d\n\n", input_parameter.phases.at(0).length, input_parameter.phases.at(1).length, std::size(gen_environment.cpu_view()), PAGE_SIZE);
#endif // PRINT_STATISTICS_INTO_FILE

    auto phase_stats = champsim::main(gen_environment, input_parameter.phases, input_parameter.traces);

#if (USE_VCPKG == ENABLE)
    fmt::print("\nChampSim completed all CPUs\n\n");
#endif // USE_VCPKG
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "\nChampSim completed all CPUs\n\n");
#endif // PRINT_STATISTICS_INTO_FILE

#if (CPU_USE_MULTIPLE_CORES == ENABLE)
    /** @todo Waiting for updating*/

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

#endif // RAMULATOR

#else
/* Original code of ChampSim */

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
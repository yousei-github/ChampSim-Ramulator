#include "main.h"

/* Defines and Declaration for ChampSim */
uint8_t warmup_complete[NUM_CPUS] = {}, simulation_complete[NUM_CPUS] = {}, all_warmup_complete = 0, all_simulation_complete = 0,
MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS, knob_cloudsuite = 0, knob_low_bandwidth = 0;

uint64_t warmup_instructions = 1000000, simulation_instructions = 10000000;

auto start_time = time(NULL);

#if (USER_CODES == ENABLE)
// initialize knobs
uint8_t show_heartbeat = 1;
#else
extern MEMORY_CONTROLLER memory_controller;
extern std::array<O3_CPU*, NUM_CPUS> ooo_cpu;
extern std::array<CACHE*, NUM_CACHES> caches;
extern std::array<champsim::operable*, NUM_OPERABLES> operables;
#endif  // USER_CODES

// For backwards compatibility with older module source.
champsim::deprecated_clock_cycle current_core_cycle;

std::vector<tracereader*> traces;

void record_roi_stats(uint32_t cpu, CACHE* cache);
void print_roi_stats(uint32_t cpu, CACHE* cache);
void print_sim_stats(uint32_t cpu, CACHE* cache);
void print_branch_stats(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu);

#if (USER_CODES == ENABLE)
#if (RAMULATOR == ENABLE)
#else
void print_memory_stats(MEMORY_CONTROLLER& memory_controller);
#endif  // RAMULATOR

void print_configuration_details(VirtualMemory& vmem);
#else
void print_dram_stats();
#endif  // USER_CODES

void reset_cache_stats(uint32_t cpu, CACHE* cache);
#if (USER_CODES == ENABLE)
#if (RAMULATOR == ENABLE)
void finish_warmup(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu, std::array<CACHE*, NUM_CACHES>& caches);
#else
void finish_warmup(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu, std::array<CACHE*, NUM_CACHES>& caches, MEMORY_CONTROLLER& memory_controller);
#endif  // RAMULATOR
#endif  // USER_CODES
void signal_handler(int signal);

#if (RAMULATOR == ENABLE)
/* Defines and Declaration for Ramulator */
using namespace ramulator;

#if (MEMORY_USE_HYBRID == ENABLE)
void configure_fast_memory_to_run_simulation(const std::string& standard, Config& configs, const std::string& standard2, Config& configs2);

template <typename T>
void next_configure_slow_memory_to_run_simulation(const std::string& standard2, Config& configs2, Config& configs, T* spec);

template <typename T, typename T2>
void start_run_simulation(const Config& configs, T* spec, const Config& configs2, T2* spec2);

template <typename T, typename T2>
void simulation_run(const Config& configs, Memory<T, Controller>& memory, const Config& configs2, Memory<T2, Controller>& memory2);
#else
void configure_memory_to_run_simulation(const std::string& standard, Config& configs);

template <typename T>
void start_run_simulation(const Config& configs, T* spec);

template <typename T>
void simulation_run(const Config& configs, Memory<T, Controller>& memory);
#endif  // MEMORY_USE_HYBRID
#else
void simulation_run();
#endif  // RAMULATOR

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    printf("Usage: %s --warmup_instructions <warmup-instructions> --simulation_instructions <simulation-instructions> [--stats <filename>] <configs-file> <configs-file2> <trace-filename1>\n"
           "Example: %s --warmup_instructions 1000000 --simulation_instructions 2000000 ramulator-configs1.cfg ramulator-configs2.cfg cpu_trace.xz\n",
           argv[0], argv[0]);
    return 0;
  }

#if (USE_OPENMP == ENABLE)
#pragma omp parallel
  {
    printf("(%s) Thread %d of %d threads says hello.\n", __func__, omp_get_thread_num(), omp_get_num_threads());
  }
#endif  // USE_OPENMP

  // interrupt signal hanlder
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = signal_handler;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
#else
  std::cout << std::endl << "*** ChampSim Multicore Out-of-Order Simulator ***" << std::endl << std::endl;
#endif  // PRINT_STATISTICS_INTO_FILE

#if (USER_CODES == ENABLE)
  uint64_t start_position_of_traces = 0;
#if (RAMULATOR == ENABLE)
  uint64_t start_position_of_configs = 0;
  string stats_out;
  uint8_t stats_flag = 0;
  uint8_t mapping_flag = 0;
  uint64_t start_position_of_stats = 0;
  uint64_t start_position_of_mapping = 0;
#endif  // RAMULATOR
  for (auto i = 1; i < argc; i++)
  {
    static uint8_t abort_flag = 0;
    if (strcmp(argv[i], "--warmup_instructions") == 0)
    {
      if (i + 1 < argc)
      {
        warmup_instructions = atol(argv[++i]);

#if (RAMULATOR == ENABLE)
        start_position_of_configs = i + 1;
        start_position_of_traces = start_position_of_configs + NUMBER_OF_MEMORIES;
#else
        start_position_of_traces = i + 1;
#endif  // RAMULATOR
        continue;
      }
      else
      {
        std::cout << __func__ << ": Need parameter behind --warmup_instructions." << std::endl;
        abort_flag++;
      }
    }

    if (strcmp(argv[i], "--simulation_instructions") == 0)
    {
      if (i + 1 < argc)
      {
        simulation_instructions = atol(argv[++i]);

#if (RAMULATOR == ENABLE)
        start_position_of_configs = i + 1;
        start_position_of_traces = start_position_of_configs + NUMBER_OF_MEMORIES;
#else
        start_position_of_traces = i + 1;
#endif  // RAMULATOR
        continue;
      }
      else
      {
        std::cout << __func__ << ": Need parameter behind --simulation_instructions." << std::endl;
        abort_flag++;
      }
    }

#if (RAMULATOR == ENABLE)
    if (strcmp(argv[i], "--stats") == 0)
    {
      if (i + 1 < argc)
      {
        stats_flag++;
        start_position_of_stats = ++i;

        start_position_of_configs = i + 1;
        start_position_of_traces = start_position_of_configs + NUMBER_OF_MEMORIES;
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
        mapping_flag++;
        start_position_of_mapping = ++i;

        start_position_of_configs = i + 1;
        start_position_of_traces = start_position_of_configs + NUMBER_OF_MEMORIES;
        continue;
      }
      else
      {
        std::cout << __func__ << ": Need parameter behind --mapping." << std::endl;
        abort_flag++;
      }
    }
#endif  // RAMULATOR

    if (strcmp(argv[i], "--hide_heartbeat") == 0)
    {
      show_heartbeat = 0;
      start_position_of_traces++;
      continue;
    }

    if (strcmp(argv[i], "--cloudsuite") == 0)
    {
      knob_cloudsuite = 1;
      MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
      start_position_of_traces++;
      continue;
    }

    if (abort_flag)
    {
      abort();
    }
  }
#else
  // initialize knobs
  uint8_t show_heartbeat = 1;

  // check to see if knobs changed using getopt_long()
  int traces_encountered = 0;
  static struct option long_options[] = {{"warmup_instructions", required_argument, 0, 'w'},
                                           {"simulation_instructions", required_argument, 0, 'i'},
                                           {"hide_heartbeat", no_argument, 0, 'h'},
                                           {"cloudsuite", no_argument, 0, 'c'},
                                           {"traces", no_argument, &traces_encountered, 1},
                                           {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long_only(argc, argv, "w:i:hc", long_options, NULL)) != -1 && !traces_encountered)
  {
    switch (c)
    {
    case 'w':
      warmup_instructions = atol(optarg);
      break;
    case 'i':
      simulation_instructions = atol(optarg);
      break;
    case 'h':
      show_heartbeat = 0;
      break;
    case 'c':
      knob_cloudsuite = 1;
      MAX_INSTR_DESTINATIONS = NUM_INSTR_DESTINATIONS_SPARC;
      break;
    case 0:
      break;
    default:
      abort();
    }
  }
#endif

#if (RAMULATOR == ENABLE)
  // prepare initialization for Ramulator.
#if (MEMORY_USE_HYBRID == ENABLE)
  // Read memory configuration file.
  Config configs(argv[start_position_of_configs]);      // fast memory
  Config configs2(argv[start_position_of_configs + 1]); // slow memory

  const std::string& standard = configs["standard"];
  assert(standard != "" || "DRAM standard should be specified.");
  const std::string& standard2 = configs2["standard"];
  assert(standard2 != "" || "DRAM standard should be specified.");

  configs.add("trace_type", "DRAM");
  configs2.add("trace_type", "DRAM");

  if (stats_flag == 0)
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
  if (mapping_flag == 0)
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

  if (stats_flag == 0)
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
  if (mapping_flag == 0)
  {
    configs.add("mapping", "defaultmapping");
  }
  else
  {
    configs.add("mapping", argv[start_position_of_mapping]);
  }

  configs.set_core_num(NUM_CPUS);
#endif  // MEMORY_USE_HYBRID
#endif  // RAMULATOR

#if (USER_CODES == ENABLE)
#if (PRINT_MEMORY_TRACE == ENABLE)
  // prepare file for recording memory traces.
  output_memory_trace_initialization(argv[start_position_of_traces]);
#endif  // PRINT_MEMORY_TRACE

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  output_champsim_statistics_initialization(argv[start_position_of_traces]);
  fprintf(outputchampsimstatistics.trace_file, "\n*** ChampSim Multicore Out-of-Order Simulator ***\n\n");
#endif  // PRINT_STATISTICS_INTO_FILE

#else
  std::cout << "Warmup Instructions: " << warmup_instructions << std::endl;
  std::cout << "Simulation Instructions: " << simulation_instructions << std::endl;
  std::cout << "Number of CPUs: " << NUM_CPUS << std::endl;

  long long int dram_size = DRAM_CHANNELS * DRAM_RANKS * DRAM_BANKS * DRAM_ROWS * DRAM_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MiB
  std::cout << "Off-chip DRAM Size: ";
  if (dram_size > 1024)
    std::cout << dram_size / 1024 << " GiB";
  else
    std::cout << dram_size << " MiB";
  std::cout << " Channels: " << DRAM_CHANNELS << " Width: " << 8 * DRAM_CHANNEL_WIDTH << "-bit Data Rate: " << DRAM_IO_FREQ << " MT/s" << std::endl;

  std::cout << std::endl;
  std::cout << "VirtualMemory physical capacity: " << std::size(vmem.ppage_free_list) * vmem.page_size;
  std::cout << " num_ppages: " << std::size(vmem.ppage_free_list) << std::endl;
  std::cout << "VirtualMemory page size: " << PAGE_SIZE << " log2_page_size: " << LOG2_PAGE_SIZE << std::endl;

  std::cout << std::endl;
#endif

  // start trace file setup
#if (USER_CODES == ENABLE)
  for (int64_t i = start_position_of_traces; i < argc; i++)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, "CPU %ld runs %s\n", traces.size(), argv[i]);
#else
    std::cout << "CPU " << traces.size() << " runs " << argv[i] << std::endl;
#endif  // PRINT_STATISTICS_INTO_FILE

    traces.push_back(get_tracereader(argv[i], traces.size(), knob_cloudsuite));

    if (traces.size() > NUM_CPUS)
    {
      printf("\n*** Too many traces for the configured number of cores ***\n\n");
      assert(0);
    }
  }
#else
  for (int i = optind; i < argc; i++)
  {
    std::cout << "CPU " << traces.size() << " runs " << argv[i] << std::endl;

    traces.push_back(get_tracereader(argv[i], traces.size(), knob_cloudsuite));

    if (traces.size() > NUM_CPUS)
    {
      printf("\n*** Too many traces for the configured number of cores ***\n\n");
      assert(0);
    }
  }
#endif

  if (traces.size() != NUM_CPUS)
  {
    printf("\n*** Not enough traces for the configured number of cores ***\n\n");
    assert(0);
  }
  // end trace file setup

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
  configure_fast_memory_to_run_simulation(standard, configs, standard2, configs2);
#else
  configure_memory_to_run_simulation(standard, configs);
#endif  // MEMORY_USE_HYBRID

  printf("Simulation done. Statistics written to %s\n", stats_out.c_str());
#else
  simulation_run();
  printf("Simulation done.\n");
#endif  // RAMULATOR

#if (PRINT_MEMORY_TRACE == ENABLE)
  output_memory_trace_deinitialization(outputmemorytrace_one);
#endif  // PRINT_MEMORY_TRACE

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  output_champsim_statistics_deinitialization(outputchampsimstatistics);
#endif  // PRINT_STATISTICS_INTO_FILE

  return 0;
}

#if (USER_CODES == ENABLE)
#else
uint64_t champsim::deprecated_clock_cycle::operator[](std::size_t cpu_idx)
{
  static bool deprecate_printed = false;
  if (!deprecate_printed)
  {
    std::cout << "WARNING: The use of 'current_core_cycle[cpu]' is deprecated." << std::endl;
    std::cout << "WARNING: Use 'this->current_cycle' instead." << std::endl;
    deprecate_printed = true;
  }
  return ooo_cpu[cpu_idx]->current_cycle;
}
#endif  // USER_CODES

void record_roi_stats(uint32_t cpu, CACHE* cache)
{
  for (uint32_t i = 0; i < NUM_TYPES; i++)
  {
    cache->roi_access[cpu][i] = cache->sim_access[cpu][i];
    cache->roi_hit[cpu][i] = cache->sim_hit[cpu][i];
    cache->roi_miss[cpu][i] = cache->sim_miss[cpu][i];
  }
}

void print_roi_stats(uint32_t cpu, CACHE* cache)
{
  uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

  for (uint32_t i = 0; i < NUM_TYPES; i++)
  {
    TOTAL_ACCESS += cache->roi_access[cpu][i];
    TOTAL_HIT += cache->roi_hit[cpu][i];
    TOTAL_MISS += cache->roi_miss[cpu][i];
  }

  if (TOTAL_ACCESS > 0)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, "%s TOTAL     ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), TOTAL_ACCESS, TOTAL_HIT, TOTAL_MISS);

    fprintf(outputchampsimstatistics.trace_file, "%s LOAD      ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->roi_access[cpu][0], cache->roi_hit[cpu][0], cache->roi_miss[cpu][0]);

    fprintf(outputchampsimstatistics.trace_file, "%s RFO       ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->roi_access[cpu][1], cache->roi_hit[cpu][1], cache->roi_miss[cpu][1]);

    fprintf(outputchampsimstatistics.trace_file, "%s PREFETCH  ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->roi_access[cpu][2], cache->roi_hit[cpu][2], cache->roi_miss[cpu][2]);

    fprintf(outputchampsimstatistics.trace_file, "%s WRITEBACK ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->roi_access[cpu][3], cache->roi_hit[cpu][3], cache->roi_miss[cpu][3]);

    fprintf(outputchampsimstatistics.trace_file, "%s TRANSLATION ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->roi_access[cpu][4], cache->roi_hit[cpu][4], cache->roi_miss[cpu][4]);

    fprintf(outputchampsimstatistics.trace_file, "%s PREFETCH  REQUESTED: %10ld  ISSUED: %10ld  USEFUL: %10ld  USELESS: %10ld\n", cache->NAME.c_str(), cache->pf_requested, cache->pf_issued, cache->pf_useful, cache->pf_useless);

    fprintf(outputchampsimstatistics.trace_file, "%s AVERAGE MISS LATENCY: %f cycles\n", cache->NAME.c_str(), (1.0 * (cache->total_miss_latency)) / TOTAL_MISS);
#else
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT << "  MISS: " << setw(10) << TOTAL_MISS << endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->roi_access[cpu][0] << "  HIT: " << setw(10) << cache->roi_hit[cpu][0] << "  MISS: " << setw(10)
      << cache->roi_miss[cpu][0] << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->roi_access[cpu][1] << "  HIT: " << setw(10) << cache->roi_hit[cpu][1] << "  MISS: " << setw(10)
      << cache->roi_miss[cpu][1] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->roi_access[cpu][2] << "  HIT: " << setw(10) << cache->roi_hit[cpu][2] << "  MISS: " << setw(10)
      << cache->roi_miss[cpu][2] << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->roi_access[cpu][3] << "  HIT: " << setw(10) << cache->roi_hit[cpu][3] << "  MISS: " << setw(10)
      << cache->roi_miss[cpu][3] << endl;

    cout << cache->NAME;
    cout << " TRANSLATION ACCESS: " << setw(10) << cache->roi_access[cpu][4] << "  HIT: " << setw(10) << cache->roi_hit[cpu][4] << "  MISS: " << setw(10)
      << cache->roi_miss[cpu][4] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  REQUESTED: " << setw(10) << cache->pf_requested << "  ISSUED: " << setw(10) << cache->pf_issued;
    cout << "  USEFUL: " << setw(10) << cache->pf_useful << "  USELESS: " << setw(10) << cache->pf_useless << endl;

    cout << cache->NAME;
    cout << " AVERAGE MISS LATENCY: " << (1.0 * (cache->total_miss_latency)) / TOTAL_MISS << " cycles" << endl;
#endif
    // cout << " AVERAGE MISS LATENCY: " <<
    // (cache->total_miss_latency)/TOTAL_MISS << " cycles " <<
    // cache->total_miss_latency << "/" << TOTAL_MISS<< endl;
  }
}

void print_sim_stats(uint32_t cpu, CACHE* cache)
{
  uint64_t TOTAL_ACCESS = 0, TOTAL_HIT = 0, TOTAL_MISS = 0;

  for (uint32_t i = 0; i < NUM_TYPES; i++)
  {
    TOTAL_ACCESS += cache->sim_access[cpu][i];
    TOTAL_HIT += cache->sim_hit[cpu][i];
    TOTAL_MISS += cache->sim_miss[cpu][i];
  }

  if (TOTAL_ACCESS > 0)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, "%s TOTAL     ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), TOTAL_ACCESS, TOTAL_HIT, TOTAL_MISS);

    fprintf(outputchampsimstatistics.trace_file, "%s LOAD      ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->sim_access[cpu][0], cache->sim_hit[cpu][0], cache->sim_miss[cpu][0]);

    fprintf(outputchampsimstatistics.trace_file, "%s RFO       ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->sim_access[cpu][1], cache->sim_hit[cpu][1], cache->sim_miss[cpu][1]);

    fprintf(outputchampsimstatistics.trace_file, "%s PREFETCH  ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->sim_access[cpu][2], cache->sim_hit[cpu][2], cache->sim_miss[cpu][2]);

    fprintf(outputchampsimstatistics.trace_file, "%s WRITEBACK ACCESS: %10ld  HIT: %10ld  MISS: %10ld\n", cache->NAME.c_str(), cache->sim_access[cpu][3], cache->sim_hit[cpu][3], cache->sim_miss[cpu][3]);
#else
    cout << cache->NAME;
    cout << " TOTAL     ACCESS: " << setw(10) << TOTAL_ACCESS << "  HIT: " << setw(10) << TOTAL_HIT << "  MISS: " << setw(10) << TOTAL_MISS << endl;

    cout << cache->NAME;
    cout << " LOAD      ACCESS: " << setw(10) << cache->sim_access[cpu][0] << "  HIT: " << setw(10) << cache->sim_hit[cpu][0] << "  MISS: " << setw(10)
      << cache->sim_miss[cpu][0] << endl;

    cout << cache->NAME;
    cout << " RFO       ACCESS: " << setw(10) << cache->sim_access[cpu][1] << "  HIT: " << setw(10) << cache->sim_hit[cpu][1] << "  MISS: " << setw(10)
      << cache->sim_miss[cpu][1] << endl;

    cout << cache->NAME;
    cout << " PREFETCH  ACCESS: " << setw(10) << cache->sim_access[cpu][2] << "  HIT: " << setw(10) << cache->sim_hit[cpu][2] << "  MISS: " << setw(10)
      << cache->sim_miss[cpu][2] << endl;

    cout << cache->NAME;
    cout << " WRITEBACK ACCESS: " << setw(10) << cache->sim_access[cpu][3] << "  HIT: " << setw(10) << cache->sim_hit[cpu][3] << "  MISS: " << setw(10)
      << cache->sim_miss[cpu][3] << endl;
#endif
  }
}

void print_branch_stats(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu)
{
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, "\nCPU %d Branch Prediction Accuracy: %f", i, (100.0 * (ooo_cpu[i]->num_branch - ooo_cpu[i]->branch_mispredictions)) / ooo_cpu[i]->num_branch);
    fprintf(outputchampsimstatistics.trace_file, "%% MPKI: %f", (1000.0 * ooo_cpu[i]->branch_mispredictions) / (ooo_cpu[i]->num_retired - warmup_instructions));
    fprintf(outputchampsimstatistics.trace_file, " Average ROB Occupancy at Mispredict: %f\n", (1.0 * ooo_cpu[i]->total_rob_occupancy_at_branch_mispredict) / ooo_cpu[i]->branch_mispredictions);

    fprintf(outputchampsimstatistics.trace_file, "Branch type MPKI\nBRANCH_DIRECT_JUMP: %f\n", (1000.0 * ooo_cpu[i]->branch_type_misses[1] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
    fprintf(outputchampsimstatistics.trace_file, "BRANCH_INDIRECT: %f\n", (1000.0 * ooo_cpu[i]->branch_type_misses[2] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
    fprintf(outputchampsimstatistics.trace_file, "BRANCH_CONDITIONAL: %f\n", (1000.0 * ooo_cpu[i]->branch_type_misses[3] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
    fprintf(outputchampsimstatistics.trace_file, "BRANCH_DIRECT_CALL: %f\n", (1000.0 * ooo_cpu[i]->branch_type_misses[4] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
    fprintf(outputchampsimstatistics.trace_file, "BRANCH_INDIRECT_CALL: %f\n", (1000.0 * ooo_cpu[i]->branch_type_misses[5] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
    fprintf(outputchampsimstatistics.trace_file, "BRANCH_RETURN: %f\n\n", (1000.0 * ooo_cpu[i]->branch_type_misses[6] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)));
#else
    cout << endl << "CPU " << i << " Branch Prediction Accuracy: ";
    cout << (100.0 * (ooo_cpu[i]->num_branch - ooo_cpu[i]->branch_mispredictions)) / ooo_cpu[i]->num_branch;
    cout << "% MPKI: " << (1000.0 * ooo_cpu[i]->branch_mispredictions) / (ooo_cpu[i]->num_retired - warmup_instructions);
    cout << " Average ROB Occupancy at Mispredict: " << (1.0 * ooo_cpu[i]->total_rob_occupancy_at_branch_mispredict) / ooo_cpu[i]->branch_mispredictions
      << endl;

    /*
    cout << "Branch types" << endl;
    cout << "NOT_BRANCH: " << ooo_cpu[i]->total_branch_types[0] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[0])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_DIRECT_JUMP: "
    << ooo_cpu[i]->total_branch_types[1] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[1])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_INDIRECT: " <<
    ooo_cpu[i]->total_branch_types[2] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[2])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_CONDITIONAL: "
    << ooo_cpu[i]->total_branch_types[3] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[3])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_DIRECT_CALL: "
    << ooo_cpu[i]->total_branch_types[4] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[4])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_INDIRECT_CALL:
    " << ooo_cpu[i]->total_branch_types[5] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[5])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_RETURN: " <<
    ooo_cpu[i]->total_branch_types[6] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[6])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl; cout << "BRANCH_OTHER: " <<
    ooo_cpu[i]->total_branch_types[7] << " " <<
    (100.0*ooo_cpu[i]->total_branch_types[7])/(ooo_cpu[i]->num_retired -
    ooo_cpu[i]->begin_sim_instr) << "%" << endl << endl;
    */

    cout << "Branch type MPKI" << endl;
    cout << "BRANCH_DIRECT_JUMP: " << (1000.0 * ooo_cpu[i]->branch_type_misses[1] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_INDIRECT: " << (1000.0 * ooo_cpu[i]->branch_type_misses[2] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_CONDITIONAL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[3] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_DIRECT_CALL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[4] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_INDIRECT_CALL: " << (1000.0 * ooo_cpu[i]->branch_type_misses[5] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl;
    cout << "BRANCH_RETURN: " << (1000.0 * ooo_cpu[i]->branch_type_misses[6] / (ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr)) << endl << endl;
#endif
  }
}

#if (USER_CODES == ENABLE)
#if (RAMULATOR == ENABLE)
#else
void print_memory_stats(MEMORY_CONTROLLER& memory_controller)
{
  uint64_t total_congested_cycle = 0;
  uint64_t total_congested_count = 0;

  // For DDR Statistics
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nDRAM Statistics\n");
#else
  std::cout << std::endl << "DRAM Statistics" << std::endl;
#endif
  for (uint32_t i = 0; i < DDR_CHANNELS; i++)
  {

    auto& channel = memory_controller.ddr_channels[i];
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, " CHANNEL %d\n", i);
    fprintf(outputchampsimstatistics.trace_file, " RQ ROW_BUFFER_HIT: %10d  ROW_BUFFER_MISS: %10d\n", channel.RQ_ROW_BUFFER_HIT, channel.RQ_ROW_BUFFER_MISS);

    fprintf(outputchampsimstatistics.trace_file, " DBUS AVG_CONGESTED_CYCLE: ");
    if (channel.dbus_count_congested)
      fprintf(outputchampsimstatistics.trace_file, "%10f\n", ((double)channel.dbus_cycle_congested / channel.dbus_count_congested));
    else
      fprintf(outputchampsimstatistics.trace_file, "-\n");

    fprintf(outputchampsimstatistics.trace_file, " WQ ROW_BUFFER_HIT: %10d  ROW_BUFFER_MISS: %10d  FULL: %10d\n\n", channel.WQ_ROW_BUFFER_HIT, channel.WQ_ROW_BUFFER_MISS, channel.WQ_FULL);
#else
    std::cout << " CHANNEL " << i << std::endl;
    std::cout << " RQ ROW_BUFFER_HIT: " << std::setw(10) << channel.RQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.RQ_ROW_BUFFER_MISS;
    std::cout << std::endl;

    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (channel.dbus_count_congested)
      std::cout << std::setw(10) << ((double)channel.dbus_cycle_congested / channel.dbus_count_congested);
    else
      std::cout << "-";
    std::cout << std::endl;

    std::cout << " WQ ROW_BUFFER_HIT: " << std::setw(10) << channel.WQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.WQ_ROW_BUFFER_MISS << " ";
    std::cout << " FULL: " << std::setw(10) << channel.WQ_FULL;
    std::cout << std::endl;

    std::cout << std::endl;
#endif

    total_congested_cycle += channel.dbus_cycle_congested;
    total_congested_count += channel.dbus_count_congested;
  }

  if (DDR_CHANNELS > 1)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, " DBUS AVG_CONGESTED_CYCLE: ");
    if (total_congested_count)
      fprintf(outputchampsimstatistics.trace_file, "%10f\n", ((double)total_congested_cycle / total_congested_count));
    else
      fprintf(outputchampsimstatistics.trace_file, "-\n");
#else
    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (total_congested_count)
      std::cout << std::setw(10) << ((double)total_congested_cycle / total_congested_count);
    else
      std::cout << "-";

    std::cout << std::endl;
#endif
  }

  total_congested_cycle = 0;
  total_congested_count = 0;

  // For HBM Statistics
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nHBM Statistics\n");
#else
  std::cout << std::endl << "HBM Statistics" << std::endl;
#endif
  for (uint32_t i = 0; i < HBM_CHANNELS; i++)
  {
    auto& channel = memory_controller.hbm_channels[i];
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, " CHANNEL %d\n", i);
    fprintf(outputchampsimstatistics.trace_file, " RQ ROW_BUFFER_HIT: %10d  ROW_BUFFER_MISS: %10d\n", channel.RQ_ROW_BUFFER_HIT, channel.RQ_ROW_BUFFER_MISS);

    fprintf(outputchampsimstatistics.trace_file, " DBUS AVG_CONGESTED_CYCLE: ");
    if (channel.dbus_count_congested)
      fprintf(outputchampsimstatistics.trace_file, "%10f\n", ((double)channel.dbus_cycle_congested / channel.dbus_count_congested));
    else
      fprintf(outputchampsimstatistics.trace_file, "-\n");

    fprintf(outputchampsimstatistics.trace_file, " WQ ROW_BUFFER_HIT: %10d  ROW_BUFFER_MISS: %10d  FULL: %10d\n\n", channel.WQ_ROW_BUFFER_HIT, channel.WQ_ROW_BUFFER_MISS, channel.WQ_FULL);
#else
    std::cout << " CHANNEL " << i << std::endl;
    std::cout << " RQ ROW_BUFFER_HIT: " << std::setw(10) << channel.RQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.RQ_ROW_BUFFER_MISS;
    std::cout << std::endl;

    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (channel.dbus_count_congested)
      std::cout << std::setw(10) << ((double)channel.dbus_cycle_congested / channel.dbus_count_congested);
    else
      std::cout << "-";
    std::cout << std::endl;

    std::cout << " WQ ROW_BUFFER_HIT: " << std::setw(10) << channel.WQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.WQ_ROW_BUFFER_MISS << " ";
    std::cout << " FULL: " << std::setw(10) << channel.WQ_FULL;
    std::cout << std::endl;

    std::cout << std::endl;
#endif

    total_congested_cycle += channel.dbus_cycle_congested;
    total_congested_count += channel.dbus_count_congested;
  }

  if (HBM_CHANNELS > 1)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, " DBUS AVG_CONGESTED_CYCLE: ");
    if (total_congested_count)
      fprintf(outputchampsimstatistics.trace_file, "%10f\n", ((double)total_congested_cycle / total_congested_count));
    else
      fprintf(outputchampsimstatistics.trace_file, "-\n");
#else
    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (total_congested_count)
      std::cout << std::setw(10) << ((double)total_congested_cycle / total_congested_count);
    else
      std::cout << "-";

    std::cout << std::endl;
#endif
  }

  // Output memory statistics
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nAverage memory access time: %10f cycle.\n", memory_controller.get_average_memory_access_time());
#else
  std::cout << "Average memory access time: " << std::setw(10) << memory_controller.get_average_memory_access_time() << " cycle. " << std::endl;
#endif
}
#endif  // RAMULATOR

void print_configuration_details(VirtualMemory& vmem)
{
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nWarmup Instructions: %ld\n", warmup_instructions);
  fprintf(outputchampsimstatistics.trace_file, "Simulation Instructions: %ld\n", simulation_instructions);
  fprintf(outputchampsimstatistics.trace_file, "Number of CPUs: %d\n", NUM_CPUS);

#if RAMULATOR == ENABLE
#else
  uint64_t ddr_address_space = DDR_CHANNELS * DDR_RANKS * DDR_BANKS * DDR_ROWS * DDR_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MB
  uint64_t ddr_capacity = DDR_CAPACITY / 1024 / 1024; // in MB

  fprintf(outputchampsimstatistics.trace_file, "Off-chip DDR Address Space: ");

  if (ddr_address_space >= 1024)
    fprintf(outputchampsimstatistics.trace_file, "%ld GB", ddr_address_space / 1024);
  else
    fprintf(outputchampsimstatistics.trace_file, "%ld MB", ddr_address_space);

  fprintf(outputchampsimstatistics.trace_file, ", Capacity: ");
  if (ddr_capacity >= 1024)
    fprintf(outputchampsimstatistics.trace_file, "%ld GB", ddr_capacity / 1024);
  else
    fprintf(outputchampsimstatistics.trace_file, "%ld MB", ddr_capacity);

  fprintf(outputchampsimstatistics.trace_file, ", Channels: %ld, Width: %ld-bit, Data Rate: %ld MT/s\n", DDR_CHANNELS, 8 * DRAM_CHANNEL_WIDTH, DRAM_IO_FREQ);

  uint64_t hbm_address_space = HBM_CHANNELS * HBM_BANKS * HBM_ROWS * HBM_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MB
  uint64_t hbm_capacity = HBM_CAPACITY / 1024 / 1024; // in MB
  fprintf(outputchampsimstatistics.trace_file, "On-chip HBM Address Space: ");
  if (hbm_address_space >= 1024)
    fprintf(outputchampsimstatistics.trace_file, "%ld GB", hbm_address_space / 1024);
  else
    fprintf(outputchampsimstatistics.trace_file, "%ld MB", hbm_address_space);

  fprintf(outputchampsimstatistics.trace_file, ", Capacity: ");
  if (hbm_capacity >= 1024)
    fprintf(outputchampsimstatistics.trace_file, "%ld GB", hbm_capacity / 1024);
  else
    fprintf(outputchampsimstatistics.trace_file, "%ld MB", hbm_capacity);

  fprintf(outputchampsimstatistics.trace_file, ", Channels: %ld, Width: %ld-bit, Data Rate: %ld MT/s\n", HBM_CHANNELS, 8 * DRAM_CHANNEL_WIDTH, DRAM_IO_FREQ);
#endif

  fprintf(outputchampsimstatistics.trace_file, "\nVirtualMemory physical capacity: %ld B", std::size(vmem.ppage_free_list) * vmem.page_size);
  fprintf(outputchampsimstatistics.trace_file, ", num_ppages: %ld\n", std::size(vmem.ppage_free_list));
  fprintf(outputchampsimstatistics.trace_file, "VirtualMemory page size: %ld, log2_page_size: %d\n\n", PAGE_SIZE, LOG2_PAGE_SIZE);
#else
  std::cout << "Warmup Instructions: " << warmup_instructions << std::endl;
  std::cout << "Simulation Instructions: " << simulation_instructions << std::endl;
  std::cout << "Number of CPUs: " << NUM_CPUS << std::endl;

  uint64_t ddr_address_space = DDR_CHANNELS * DDR_RANKS * DDR_BANKS * DDR_ROWS * DDR_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MB
  uint64_t ddr_capacity = DDR_CAPACITY / 1024 / 1024; // in MB

  std::cout << "Off-chip DDR Address Space: ";

  if (ddr_address_space >= 1024)
    std::cout << ddr_address_space / 1024 << " GB";
  else
    std::cout << ddr_address_space << " MB";

  std::cout << ", Capacity: ";
  if (ddr_capacity >= 1024)
    std::cout << ddr_capacity / 1024 << " GB";
  else
    std::cout << ddr_capacity << " MB";
  std::cout << ", Channels: " << DDR_CHANNELS << ", Width: " << 8 * DRAM_CHANNEL_WIDTH << "-bit, Data Rate: " << DRAM_IO_FREQ << " MT/s" << std::endl;

  uint64_t hbm_address_space = HBM_CHANNELS * HBM_BANKS * HBM_ROWS * HBM_COLUMNS * BLOCK_SIZE / 1024 / 1024; // in MB
  uint64_t hbm_capacity = HBM_CAPACITY / 1024 / 1024; // in MB
  std::cout << "On-chip HBM Address Space: ";
  if (hbm_address_space >= 1024)
    std::cout << hbm_address_space / 1024 << " GB";
  else
    std::cout << hbm_address_space << " MB";

  std::cout << ", Capacity: ";
  if (hbm_capacity >= 1024)
    std::cout << hbm_capacity / 1024 << " GB";
  else
    std::cout << hbm_capacity << " MB";
  std::cout << ", Channels: " << HBM_CHANNELS << ", Width: " << 8 * DRAM_CHANNEL_WIDTH << "-bit, Data Rate: " << DRAM_IO_FREQ << " MT/s" << std::endl;

  std::cout << std::endl;
  std::cout << "VirtualMemory physical capacity: " << std::size(vmem.ppage_free_list) * vmem.page_size;
  std::cout << ", num_ppages: " << std::size(vmem.ppage_free_list) << std::endl;
  std::cout << "VirtualMemory page size: " << PAGE_SIZE << ", log2_page_size: " << LOG2_PAGE_SIZE << std::endl;

  std::cout << std::endl;
#endif
}

#else
void print_dram_stats()
{
  uint64_t total_congested_cycle = 0;
  uint64_t total_congested_count = 0;

  std::cout << std::endl;
  std::cout << "DRAM Statistics" << std::endl;
  for (uint32_t i = 0; i < DRAM_CHANNELS; i++)
  {
    std::cout << " CHANNEL " << i << std::endl;

    auto& channel = memory_controller.dram_channels[i];
    std::cout << " RQ ROW_BUFFER_HIT: " << std::setw(10) << channel.RQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.RQ_ROW_BUFFER_MISS;
    std::cout << std::endl;

    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (channel.dbus_count_congested)
      std::cout << std::setw(10) << ((double)channel.dbus_cycle_congested / channel.dbus_count_congested);
    else
      std::cout << "-";
    std::cout << std::endl;

    std::cout << " WQ ROW_BUFFER_HIT: " << std::setw(10) << channel.WQ_ROW_BUFFER_HIT << " ";
    std::cout << " ROW_BUFFER_MISS: " << std::setw(10) << channel.WQ_ROW_BUFFER_MISS << " ";
    std::cout << " FULL: " << std::setw(10) << channel.WQ_FULL;
    std::cout << std::endl;

    std::cout << std::endl;

    total_congested_cycle += channel.dbus_cycle_congested;
    total_congested_count += channel.dbus_count_congested;
  }



  if (DRAM_CHANNELS > 1)
  {
    std::cout << " DBUS AVG_CONGESTED_CYCLE: ";
    if (total_congested_count)
      std::cout << std::setw(10) << ((double)total_congested_cycle / total_congested_count);
    else
      std::cout << "-";

    std::cout << std::endl;
  }
}
#endif  // USER_CODES

void reset_cache_stats(uint32_t cpu, CACHE* cache)
{
  for (uint32_t i = 0; i < NUM_TYPES; i++)
  {
    cache->sim_access[cpu][i] = 0;
    cache->sim_hit[cpu][i] = 0;
    cache->sim_miss[cpu][i] = 0;
  }

  cache->pf_requested = 0;
  cache->pf_issued = 0;
  cache->pf_useful = 0;
  cache->pf_useless = 0;
  cache->pf_fill = 0;

  cache->total_miss_latency = 0;

  cache->RQ_ACCESS = 0;
  cache->RQ_MERGED = 0;
  cache->RQ_TO_CACHE = 0;

  cache->WQ_ACCESS = 0;
  cache->WQ_MERGED = 0;
  cache->WQ_TO_CACHE = 0;
  cache->WQ_FORWARD = 0;
  cache->WQ_FULL = 0;
}

#if (USER_CODES == ENABLE)
#if (RAMULATOR == ENABLE)
void finish_warmup(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu, std::array<CACHE*, NUM_CACHES>& caches)
#else
void finish_warmup(std::array<O3_CPU*, NUM_CPUS>& ooo_cpu, std::array<CACHE*, NUM_CACHES>& caches, MEMORY_CONTROLLER& memory_controller)
#endif
#endif
{
  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

  // reset core latency
  // note: since re-ordering he function calls in the main simulation loop, it's
  // no longer necessary to add
  //       extra latency for scheduling and execution, unless you want these
  //       steps to take longer than 1 cycle.
  // PAGE_TABLE_LATENCY = 100;
  // SWAP_LATENCY = 100000;

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\n");
#else
  cout << endl;
#endif
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    fprintf(outputchampsimstatistics.trace_file, "Warmup complete CPU %d instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
    fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
#else
    cout << "Warmup complete CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
    cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << endl;
#endif

    ooo_cpu[i]->begin_sim_cycle = ooo_cpu[i]->current_cycle;
    ooo_cpu[i]->begin_sim_instr = ooo_cpu[i]->num_retired;

    // reset branch stats
    ooo_cpu[i]->num_branch = 0;
    ooo_cpu[i]->branch_mispredictions = 0;
    ooo_cpu[i]->total_rob_occupancy_at_branch_mispredict = 0;

    for (uint32_t j = 0; j < 8; j++)
    {
      ooo_cpu[i]->total_branch_types[j] = 0;
      ooo_cpu[i]->branch_type_misses[j] = 0;
    }

    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      reset_cache_stats(i, *it);
  }
#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\n");
#else
  cout << endl;
#endif

#if (USER_CODES == ENABLE)
#if (RAMULATOR == ENABLE)
#else
  // reset DDR stats
  for (uint32_t i = 0; i < DDR_CHANNELS; i++)
  {
    memory_controller.ddr_channels[i].WQ_ROW_BUFFER_HIT = 0;
    memory_controller.ddr_channels[i].WQ_ROW_BUFFER_MISS = 0;
    memory_controller.ddr_channels[i].RQ_ROW_BUFFER_HIT = 0;
    memory_controller.ddr_channels[i].RQ_ROW_BUFFER_MISS = 0;
  }
  // reset HBM stats
  for (uint32_t i = 0; i < HBM_CHANNELS; i++)
  {
    memory_controller.hbm_channels[i].WQ_ROW_BUFFER_HIT = 0;
    memory_controller.hbm_channels[i].WQ_ROW_BUFFER_MISS = 0;
    memory_controller.hbm_channels[i].RQ_ROW_BUFFER_HIT = 0;
    memory_controller.hbm_channels[i].RQ_ROW_BUFFER_MISS = 0;
  }
#endif  // RAMULATOR
#else
  // reset DRAM stats
  for (uint32_t i = 0; i < DRAM_CHANNELS; i++)
  {
    memory_controller.dram_channels[i].WQ_ROW_BUFFER_HIT = 0;
    memory_controller.dram_channels[i].WQ_ROW_BUFFER_MISS = 0;
    memory_controller.dram_channels[i].RQ_ROW_BUFFER_HIT = 0;
    memory_controller.dram_channels[i].RQ_ROW_BUFFER_MISS = 0;
  }
#endif  // USER_CODES
}

void signal_handler(int signal)
{
  cout << "Caught signal: " << signal << endl;
  exit(1);
}

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
void configure_fast_memory_to_run_simulation
(const std::string& standard, Config& configs,
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

template <typename T>
void next_configure_slow_memory_to_run_simulation
(const std::string& standard2, Config& configs2,
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

template <typename T, typename T2>
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
    channel->id = c;
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
    channel->id = c;
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

template <typename T, typename T2>
void simulation_run(const Config& configs, Memory<T, Controller>& memory, const Config& configs2, Memory<T2, Controller>& memory2)
{
  VirtualMemory vmem(memory.max_address + memory2.max_address, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

  std::array<O3_CPU*, NUM_CPUS> ooo_cpu{};
  MEMORY_CONTROLLER<T, T2> memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, CPU_FREQUENCY / memory2.spec->speed_entry.freq, memory, memory2);
  CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
            LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
                 L2C_PREF_ACTIVATE_MASK, &LLC, CPU0_L2C_PREFETCHER, CPU0_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
                 L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1D_PREFETCHER, CPU0_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
  PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
  CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
                  STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU0_STLB_PREFETCHER, CPU0_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
                 L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1I_PREFETCHER, CPU0_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
                  ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_ITLB_PREFETCHER, CPU0_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
                  DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_DTLB_PREFETCHER, CPU0_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
              CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
              BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

  // put cpu into the cpu array.
  ooo_cpu.at(NUM_CPUS - 1) = &cpu0;

  std::array<CACHE*, NUM_CACHES> caches{&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB};

  std::array<champsim::operable*, NUM_OPERABLES> operables{&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &memory_controller};

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
    data1.to_return = data2.to_return = {(MemoryRequestProducer*)(&LLC)};
    memory_controller.operate();

    // if (count == 150) // 100
    // {
    //   memory_controller.start_swapping_segments(0 << LOG2_BLOCK_SIZE, 64 << LOG2_BLOCK_SIZE, 1);
    // }

    // issue read and write
#define DATA1_TO_WRITE (200)
#define DATA2_TO_WRITE (201)
#define DATA1_ADDRESS  (0 << LOG2_BLOCK_SIZE)   // 2
#define DATA2_ADDRESS  (64 << LOG2_BLOCK_SIZE)  // 66
    if (count == 100) // 150
    {
      int32_t status = 0;
      data1.address = DATA1_ADDRESS;
      data2.address = DATA2_ADDRESS;

      // read
      status = memory_controller.add_rq(&data1);
      status = memory_controller.add_rq(&data2);

      // write
      data1.data = DATA1_TO_WRITE;
      data2.data = DATA2_TO_WRITE;
      status = memory_controller.add_wq(&data1);
      status = memory_controller.add_wq(&data2);

      printf("status is %d\n", status);
    }

    // if (count == 150) // 150
    // {
    //   PACKET data3, data4;
    //   int32_t status = 0;
    //   data3.address = DATA1_ADDRESS;
    //   data4.address = DATA2_ADDRESS;
    //   data3.cpu = data4.cpu = 0;
    //   data3.to_return = data4.to_return = {(MemoryRequestProducer*)(&cpu0_L2C)};

    //   // read
    //   status = memory_controller.add_rq(&data3);
    //   status = memory_controller.add_rq(&data4);

    //   // write
    //   data3.data = DATA1_TO_WRITE;
    //   data4.data = DATA2_TO_WRITE;
    //   status = memory_controller.add_wq(&data3);
    //   status = memory_controller.add_wq(&data4);

    //   printf("status is %d\n", status);
    // }

    // if (count == 222)
    // {
    //   int32_t status = 0;
    //   data1.address = DATA1_ADDRESS;
    //   data2.address = DATA2_ADDRESS;

    //   // read
    //   status = memory_controller.add_rq(&data1);
    //   status = memory_controller.add_rq(&data2);

    //   // write
    //   data1.data = DATA1_TO_WRITE + 2;
    //   data2.data = DATA2_TO_WRITE + 2;
    //   status = memory_controller.add_wq(&data1);
    //   status = memory_controller.add_wq(&data2);

    //   printf("status is %d\n", status);
    // }
    if (count == 10072)
    {
      int32_t status = 0;
      data1.address = DATA1_ADDRESS;
      data2.address = DATA2_ADDRESS;

      // read
      status = memory_controller.add_rq(&data1);
      status = memory_controller.add_rq(&data2);

      // write
      data1.data = DATA1_TO_WRITE + 4;
      data2.data = DATA2_TO_WRITE + 4;
      status = memory_controller.add_wq(&data1);
      status = memory_controller.add_wq(&data2);

      printf("status is %d\n", status);
    }

    if (count == 10000)
    {
      memory_controller.start_swapping_segments(0 << LOG2_BLOCK_SIZE, 64 << LOG2_BLOCK_SIZE, 2);
    }

    if (count == 20000)
    {
      memory_controller.start_swapping_segments(0 << LOG2_BLOCK_SIZE, 64 << LOG2_BLOCK_SIZE, 64);
    }

    count++;
    if (count >= warmup_instructions + simulation_instructions)
    {
      exit(0);
    }
  }
#endif  // MEMORY_USE_SWAPPING_UNIT && TEST_SWAPPING_UNIT

  // simulation entry point
  while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>()))
  {
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour * 60;
    elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

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
    //std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

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
        //fprintf(outputchampsimstatistics.trace_file, "Heartbeat CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
        //fprintf(outputchampsimstatistics.trace_file, " heartbeat IPC: %f cumulative IPC: %f", heartbeat_ipc, cumulative_ipc);
        //fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#else
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

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
      if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0)
          && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions)))
      {
        simulation_complete[i] = 1;
        ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
        ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        fprintf(outputchampsimstatistics.trace_file, "Finished CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
        fprintf(outputchampsimstatistics.trace_file, " cumulative IPC: %f", ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
        fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
#else
        std::cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
        std::cout << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
          record_roi_stats(i, *it);
      }
    }
  }

  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  memory2.finish();
  Stats::statlist.printall();

  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nChampSim completed all CPUs\n");
  if (NUM_CPUS > 1)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nTotal Simulation Statistics (not including warmup)\n");
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle));
      fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr, ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  fprintf(outputchampsimstatistics.trace_file, "\nRegion of Interest Statistics\n");
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
    fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#else
  std::cout << std::endl << "ChampSim completed all CPUs" << std::endl;
  if (NUM_CPUS > 1)
  {
    std::cout << std::endl << "Total Simulation Statistics (not including warmup)" << std::endl;
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      std::cout << std::endl
        << "CPU " << i
        << " cumulative IPC: " << (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      std::cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
        << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << std::endl;
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  std::cout << std::endl << "Region of Interest Statistics" << std::endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    std::cout << std::endl << "CPU " << i << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
    std::cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << std::endl;
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#endif

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_prefetcher_final_stats();

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_replacement_final_stats();

#ifndef CRC2_COMPILE
  print_branch_stats(ooo_cpu);
#endif
}
#else
void configure_memory_to_run_simulation
(const std::string& standard, Config& configs)
{
  if (standard == "DDR3")
  {
    DDR3* ddr3 = new DDR3(configs["org"], configs["speed"]);
    start_run_simulation(configs, ddr3);
  }
  else if (standard == "DDR4")
  {
    DDR4* ddr4 = new DDR4(configs["org"], configs["speed"]);
    start_run_simulation(configs, ddr4);
  }
  else if (standard == "SALP-MASA")
  {
    SALP* salp8 = new SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
    start_run_simulation(configs, salp8);
  }
  else if (standard == "LPDDR3")
  {
    LPDDR3* lpddr3 = new LPDDR3(configs["org"], configs["speed"]);
    start_run_simulation(configs, lpddr3);
  }
  else if (standard == "LPDDR4")
  {
    // total cap: 2GB, 1/2 of others
    LPDDR4* lpddr4 = new LPDDR4(configs["org"], configs["speed"]);
    start_run_simulation(configs, lpddr4);
  }
  else if (standard == "GDDR5")
  {
    GDDR5* gddr5 = new GDDR5(configs["org"], configs["speed"]);
    start_run_simulation(configs, gddr5);
  }
  else if (standard == "HBM")
  {
    HBM* hbm = new HBM(configs["org"], configs["speed"]);
    start_run_simulation(configs, hbm);
  }
  else if (standard == "WideIO")
  {
    // total cap: 1GB, 1/4 of others
    WideIO* wio = new WideIO(configs["org"], configs["speed"]);
    start_run_simulation(configs, wio);
  }
  else if (standard == "WideIO2")
  {
    // total cap: 2GB, 1/2 of others
    WideIO2* wio2 = new WideIO2(configs["org"], configs["speed"], configs.get_channels());
    wio2->channel_width *= 2;
    start_run_simulation(configs, wio2);
  }
  else if (standard == "STTMRAM")
  {
    STTMRAM* sttmram = new STTMRAM(configs["org"], configs["speed"]);
    start_run_simulation(configs, sttmram);
  }
  else if (standard == "PCM")
  {
    PCM* pcm = new PCM(configs["org"], configs["speed"]);
    start_run_simulation(configs, pcm);
  }
  // Various refresh mechanisms
  else if (standard == "DSARP")
  {
    DSARP* dsddr3_dsarp = new DSARP(configs["org"], configs["speed"], DSARP::Type::DSARP, configs.get_subarrays());
    start_run_simulation(configs, dsddr3_dsarp);
  }
  else if (standard == "ALDRAM")
  {
    ALDRAM* aldram = new ALDRAM(configs["org"], configs["speed"]);
    start_run_simulation(configs, aldram);
  }
  else if (standard == "TLDRAM")
  {
    TLDRAM* tldram = new TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
    start_run_simulation(configs, tldram);
  }
}

template <typename T>
void start_run_simulation(const Config& configs, T* spec)
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
    channel->id = c;
    channel->regStats("");
    Controller<T>* ctrl = new Controller<T>(configs, channel);
    ctrls.push_back(ctrl);
  }
  Memory<T, Controller> memory(configs, ctrls);

  if (configs["trace_type"] == "DRAM")
  {
    simulation_run(configs, memory);
  }
  else
  {
    printf("%s: Error!\n", __FUNCTION__);
  }
}

template <typename T>
void simulation_run(const Config& configs, Memory<T, Controller>& memory)
{
  VirtualMemory vmem(memory.max_address, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

  std::array<O3_CPU*, NUM_CPUS> ooo_cpu{};
  MEMORY_CONTROLLER<T> memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE, CPU_FREQUENCY / memory.spec->speed_entry.freq, memory);
  CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
            LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
                 L2C_PREF_ACTIVATE_MASK, &LLC, CPU0_L2C_PREFETCHER, CPU0_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
                 L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1D_PREFETCHER, CPU0_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
  PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
  CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
                  STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU0_STLB_PREFETCHER, CPU0_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
                 L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1I_PREFETCHER, CPU0_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
                  ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_ITLB_PREFETCHER, CPU0_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
                  DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_DTLB_PREFETCHER, CPU0_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
              CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
              BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

  // put cpu into the cpu array.
  ooo_cpu.at(NUM_CPUS - 1) = &cpu0;

  std::array<CACHE*, NUM_CACHES> caches{&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB};

  std::array<champsim::operable*, NUM_OPERABLES> operables{&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &memory_controller};

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

  // simulation entry point
  while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>()))
  {
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour * 60;
    elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

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
    //std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

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
        //fprintf(outputchampsimstatistics.trace_file, "Heartbeat CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
        //fprintf(outputchampsimstatistics.trace_file, " heartbeat IPC: %f cumulative IPC: %f", heartbeat_ipc, cumulative_ipc);
        //fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#else
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

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
      if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0)
          && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions)))
      {
        simulation_complete[i] = 1;
        ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
        ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        fprintf(outputchampsimstatistics.trace_file, "Finished CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
        fprintf(outputchampsimstatistics.trace_file, " cumulative IPC: %f", ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
        fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
#else
        std::cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
        std::cout << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
          record_roi_stats(i, *it);
      }
    }
  }

  // This a workaround for statistics set only initially lost in the end
  memory.finish();
  Stats::statlist.printall();

  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nChampSim completed all CPUs\n");
  if (NUM_CPUS > 1)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nTotal Simulation Statistics (not including warmup)\n");
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle));
      fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr, ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  fprintf(outputchampsimstatistics.trace_file, "\nRegion of Interest Statistics\n");
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
    fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#else
  std::cout << std::endl << "ChampSim completed all CPUs" << std::endl;
  if (NUM_CPUS > 1)
  {
    std::cout << std::endl << "Total Simulation Statistics (not including warmup)" << std::endl;
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      std::cout << std::endl
        << "CPU " << i
        << " cumulative IPC: " << (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      std::cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
        << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << std::endl;
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  std::cout << std::endl << "Region of Interest Statistics" << std::endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    std::cout << std::endl << "CPU " << i << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
    std::cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << std::endl;
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#endif

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_prefetcher_final_stats();

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_replacement_final_stats();

#ifndef CRC2_COMPILE
  print_branch_stats(ooo_cpu);
#endif
}
#endif  // MEMORY_USE_HYBRID
#else
void simulation_run()
{
  VirtualMemory vmem(MEMORY_CAPACITY, PAGE_SIZE, PAGE_TABLE_LEVELS, 1, MINOR_FAULT_PENALTY);

  std::array<O3_CPU*, NUM_CPUS> ooo_cpu{};
  MEMORY_CONTROLLER memory_controller(MEMORY_CONTROLLER_CLOCK_SCALE);
  CACHE LLC("LLC", CACHE_CLOCK_SCALE, LLC_LEVEL, LLC_SETS, LLC_WAYS, LLC_WQ_SIZE, LLC_RQ_SIZE, LLC_PQ_SIZE, LLC_MSHR_SIZE, LLC_LATENCY - 1, LLC_FILL_LATENCY, LLC_MAX_READ, LLC_MAX_WRITE, LOG2_BLOCK_SIZE, LLC_PREFETCH_AS_LOAD, LLC_WQ_FULL_ADDRESS, LLC_VIRTUAL_PREFETCH,
            LLC_PREF_ACTIVATE_MASK, &memory_controller, LLC_PREFETCHER, LLC_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L2C("cpu0_L2C", CACHE_CLOCK_SCALE, L2C_LEVEL, L2C_SETS, L2C_WAYS, L2C_WQ_SIZE, L2C_RQ_SIZE, L2C_PQ_SIZE, L2C_MSHR_SIZE, L2C_LATENCY - 1, L2C_FILL_LATENCY, L2C_MAX_READ, L2C_MAX_WRITE, LOG2_BLOCK_SIZE, L2C_PREFETCH_AS_LOAD, L2C_WQ_FULL_ADDRESS, L2C_VIRTUAL_PREFETCH,
                 L2C_PREF_ACTIVATE_MASK, &LLC, CPU0_L2C_PREFETCHER, CPU0_L2C_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1D("cpu0_L1D", CACHE_CLOCK_SCALE, L1D_LEVEL, L1D_SETS, L1D_WAYS, L1D_WQ_SIZE, L1D_RQ_SIZE, L1D_PQ_SIZE, L1D_MSHR_SIZE, L1D_LATENCY - 1, L1D_FILL_LATENCY, L1D_MAX_READ, L1D_MAX_WRITE, LOG2_BLOCK_SIZE, L1D_PREFETCH_AS_LOAD, L1D_WQ_FULL_ADDRESS, L1D_VIRTUAL_PREFETCH,
                 L1D_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1D_PREFETCHER, CPU0_L1D_REPLACEMENT_POLICY, ooo_cpu, vmem);
  PageTableWalker cpu0_PTW("cpu0_PTW", PTW_CPU, PTW_LEVEL, PTW_PSCL5_SET, PTW_PSCL5_WAY, PTW_PSCL4_SET, PTW_PSCL4_WAY, PTW_PSCL3_SET, PTW_PSCL3_WAY, PTW_PSCL2_SET, PTW_PSCL2_WAY, PTW_RQ_SIZE, PTW_MSHR_SIZE, PTW_MAX_READ, PTW_MAX_WRITE, PTW_LATENCY - 1, &cpu0_L1D, vmem);
  CACHE cpu0_STLB("cpu0_STLB", CACHE_CLOCK_SCALE, STLB_LEVEL, STLB_SETS, STLB_WAYS, STLB_WQ_SIZE, STLB_RQ_SIZE, STLB_PQ_SIZE, STLB_MSHR_SIZE, STLB_LATENCY - 1, STLB_FILL_LATENCY, STLB_MAX_READ, STLB_MAX_WRITE, LOG2_PAGE_SIZE, STLB_PREFETCH_AS_LOAD, STLB_WQ_FULL_ADDRESS, STLB_VIRTUAL_PREFETCH,
                  STLB_PREF_ACTIVATE_MASK, &cpu0_PTW, CPU0_STLB_PREFETCHER, CPU0_STLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_L1I("cpu0_L1I", CACHE_CLOCK_SCALE, L1I_LEVEL, L1I_SETS, L1I_WAYS, L1I_WQ_SIZE, L1I_RQ_SIZE, L1I_PQ_SIZE, L1I_MSHR_SIZE, L1I_LATENCY - 1, L1I_FILL_LATENCY, L1I_MAX_READ, L1I_MAX_WRITE, LOG2_BLOCK_SIZE, L1I_PREFETCH_AS_LOAD, L1I_WQ_FULL_ADDRESS, L1I_VIRTUAL_PREFETCH,
                 L1I_PREF_ACTIVATE_MASK, &cpu0_L2C, CPU0_L1I_PREFETCHER, CPU0_L1I_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_ITLB("cpu0_ITLB", CACHE_CLOCK_SCALE, ITLB_LEVEL, ITLB_SETS, ITLB_WAYS, ITLB_WQ_SIZE, ITLB_RQ_SIZE, ITLB_PQ_SIZE, ITLB_MSHR_SIZE, ITLB_LATENCY - 1, ITLB_FILL_LATENCY, ITLB_MAX_READ, ITLB_MAX_WRITE, LOG2_PAGE_SIZE, ITLB_PREFETCH_AS_LOAD, ITLB_WQ_FULL_ADDRESS, ITLB_VIRTUAL_PREFETCH,
                  ITLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_ITLB_PREFETCHER, CPU0_ITLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  CACHE cpu0_DTLB("cpu0_DTLB", CACHE_CLOCK_SCALE, DTLB_LEVEL, DTLB_SETS, DTLB_WAYS, DTLB_WQ_SIZE, DTLB_RQ_SIZE, DTLB_PQ_SIZE, DTLB_MSHR_SIZE, DTLB_LATENCY - 1, DTLB_FILL_LATENCY, DTLB_MAX_READ, DTLB_MAX_WRITE, LOG2_PAGE_SIZE, DTLB_PREFETCH_AS_LOAD, DTLB_WQ_FULL_ADDRESS, DTLB_VIRTUAL_PREFETCH,
                  DTLB_PREF_ACTIVATE_MASK, &cpu0_STLB, CPU0_DTLB_PREFETCHER, CPU0_DTLB_REPLACEMENT_POLICY, ooo_cpu, vmem);
  O3_CPU cpu0(CPU_0, O3_CPU_CLOCK_SCALE, CPU_DIB_SET, CPU_DIB_WAY, CPU_DIB_WINDOW, CPU_IFETCH_BUFFER_SIZE, CPU_DECODE_BUFFER_SIZE, CPU_DISPATCH_BUFFER_SIZE, CPU_ROB_SIZE, CPU_LQ_SIZE, CPU_SQ_SIZE, CPU_FETCH_WIDTH, CPU_DECODE_WIDTH, CPU_DISPATCH_WIDTH,
              CPU_SCHEDULER_SIZE, CPU_EXECUTE_WIDTH, CPU_LQ_WIDTH, CPU_SQ_WIDTH, CPU_RETIRE_WIDTH, CPU_MISPREDICT_PENALTY, CPU_DECODE_LATENCY, CPU_DISPATCH_LATENCY, CPU_SCHEDULE_LATENCY, CPU_EXECUTE_LATENCY, &cpu0_ITLB, &cpu0_DTLB, &cpu0_L1I, &cpu0_L1D,
              BRANCH_PREDICTOR, BRANCH_TARGET_BUFFER, INSTRUCTION_PREFETCHER);

  // put cpu into the cpu array.
  ooo_cpu.at(NUM_CPUS - 1) = &cpu0;

  std::array<CACHE*, NUM_CACHES> caches{&LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB};

  std::array<champsim::operable*, NUM_OPERABLES> operables{&cpu0, &LLC, &cpu0_L2C, &cpu0_L1D, &cpu0_PTW, &cpu0_STLB, &cpu0_L1I, &cpu0_ITLB, &cpu0_DTLB, &memory_controller};

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

  // simulation entry point
  while (std::any_of(std::begin(simulation_complete), std::end(simulation_complete), std::logical_not<uint8_t>()))
  {
    uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
    elapsed_minute -= elapsed_hour * 60;
    elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

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
    std::sort(std::begin(operables), std::end(operables), champsim::by_next_operate());

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
        //fprintf(outputchampsimstatistics.trace_file, "Heartbeat CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->num_retired, ooo_cpu[i]->current_cycle);
        //fprintf(outputchampsimstatistics.trace_file, " heartbeat IPC: %f cumulative IPC: %f", heartbeat_ipc, cumulative_ipc);
        //fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#else
        std::cout << "Heartbeat CPU " << i << " instructions: " << ooo_cpu[i]->num_retired << " cycles: " << ooo_cpu[i]->current_cycle;
        std::cout << " heartbeat IPC: " << heartbeat_ipc << " cumulative IPC: " << cumulative_ipc;
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

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
        finish_warmup(ooo_cpu, caches, memory_controller);
      }

      // simulation complete
      if ((all_warmup_complete > NUM_CPUS) && (simulation_complete[i] == 0)
          && (ooo_cpu[i]->num_retired >= (ooo_cpu[i]->begin_sim_instr + simulation_instructions)))
      {
        simulation_complete[i] = 1;
        ooo_cpu[i]->finish_sim_instr = ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr;
        ooo_cpu[i]->finish_sim_cycle = ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle;

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
        fprintf(outputchampsimstatistics.trace_file, "Finished CPU %ld instructions: %ld cycles: %ld", i, ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
        fprintf(outputchampsimstatistics.trace_file, " cumulative IPC: %f", ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
        fprintf(outputchampsimstatistics.trace_file, " (Simulation time: %ld hr %ld min %ld sec) \n", elapsed_hour, elapsed_minute, elapsed_second);
#else
        std::cout << "Finished CPU " << i << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle;
        std::cout << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
        std::cout << " (Simulation time: " << elapsed_hour << " hr " << elapsed_minute << " min " << elapsed_second << " sec) " << std::endl;
#endif

        for (auto it = caches.rbegin(); it != caches.rend(); ++it)
          record_roi_stats(i, *it);
      }
    }
  }

  uint64_t elapsed_second = (uint64_t)(time(NULL) - start_time), elapsed_minute = elapsed_second / 60, elapsed_hour = elapsed_minute / 60;
  elapsed_minute -= elapsed_hour * 60;
  elapsed_second -= (elapsed_hour * 3600 + elapsed_minute * 60);

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
  fprintf(outputchampsimstatistics.trace_file, "\nChampSim completed all CPUs\n");
  if (NUM_CPUS > 1)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nTotal Simulation Statistics (not including warmup)\n");
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle));
      fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr, ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  fprintf(outputchampsimstatistics.trace_file, "\nRegion of Interest Statistics\n");
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    fprintf(outputchampsimstatistics.trace_file, "\nCPU %d cumulative IPC: %f", i, ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle));
    fprintf(outputchampsimstatistics.trace_file, " instructions: %ld cycles: %ld\n", ooo_cpu[i]->finish_sim_instr, ooo_cpu[i]->finish_sim_cycle);
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#else
  std::cout << std::endl << "ChampSim completed all CPUs" << std::endl;
  if (NUM_CPUS > 1)
  {
    std::cout << std::endl << "Total Simulation Statistics (not including warmup)" << std::endl;
    for (uint32_t i = 0; i < NUM_CPUS; i++)
    {
      std::cout << std::endl
        << "CPU " << i
        << " cumulative IPC: " << (float)(ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr) / (ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle);
      std::cout << " instructions: " << ooo_cpu[i]->num_retired - ooo_cpu[i]->begin_sim_instr
        << " cycles: " << ooo_cpu[i]->current_cycle - ooo_cpu[i]->begin_sim_cycle << std::endl;
      for (auto it = caches.rbegin(); it != caches.rend(); ++it)
        print_sim_stats(i, *it);
    }
  }

  std::cout << std::endl << "Region of Interest Statistics" << std::endl;
  for (uint32_t i = 0; i < NUM_CPUS; i++)
  {
    std::cout << std::endl << "CPU " << i << " cumulative IPC: " << ((float)ooo_cpu[i]->finish_sim_instr / ooo_cpu[i]->finish_sim_cycle);
    std::cout << " instructions: " << ooo_cpu[i]->finish_sim_instr << " cycles: " << ooo_cpu[i]->finish_sim_cycle << std::endl;
    for (auto it = caches.rbegin(); it != caches.rend(); ++it)
      print_roi_stats(i, *it);
  }
#endif  // PRINT_STATISTICS_INTO_FILE

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_prefetcher_final_stats();

  for (auto it = caches.rbegin(); it != caches.rend(); ++it)
    (*it)->impl_replacement_final_stats();

#ifndef CRC2_COMPILE
#if (USER_CODES == ENABLE)
  print_memory_stats(memory_controller);
#else
  print_dram_stats();
#endif

  print_branch_stats(ooo_cpu);
#endif  // CRC2_COMPILE
}
#endif  // RAMULATOR

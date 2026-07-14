<p align="center">
  <h1 align="center"> ChampSim + Ramulator </h1>
</p>

# About This Project
This project is based on the [ChampSim](https://github.com/yousei-github/ChampSim) (Commit: [06de8d3](https://github.com/ChampSim/ChampSim/commit/06de8d3d03d9ba39b4726166aa6364881812365f)), [Ramulator](https://github.com/yousei-github/ramulator) (Commit: [743b940](https://github.com/CMU-SAFARI/ramulator/commit/743b940b70a8e18bcffb14eec22d2ed731059540)), [Ramulator2](https://github.com/CMU-SAFARI/ramulator2) (Commit: [be93be7](https://github.com/yousei-github/ramulator2/commit/be93be78055d922aa1d4d33e15bcc8f2b0c61a9d)), and it can be modified to simulate hybrid memory systems. You can modify the preprocessors in the [./include/ProjectConfiguration.h](include/ProjectConfiguration.h) file and recompile this project to try different functionalities. For example,
- Set the preprocessor `RAMULATOR` to `ENABLE` to enable Ramulator 1.0 (legacy, `.cfg`-configured) or to `DISABLE` for just using ChampSim.
- Set the preprocessor `RAMULATOR2` to `ENABLE` to enable Ramulator 2.0 (modular, YAML-configured). It is mutually exclusive with `RAMULATOR`.
- Set the preprocessor `MEMORY_USE_HYBRID` to `ENABLE` to enable hybrid memory systems or to `DISABLE` to enable single memory systems.
- Set the preprocessor `CPU_USE_MULTIPLE_CORES` to `ENABLE` to enable multiple cores to run the simulation. Note that you also need to add multiple trace paths to run this simulator.
- Set the preprocessor `PRINT_STATISTICS_INTO_FILE` to `ENABLE` for printing statistics into the `.statistics` file.
- Set the preprocessor `PRINT_MEMORY_TRACE` to `ENABLE` for printing the memory trace into the `.trace` file. Each line in the trace file represents a memory request, with the hexadecimal address followed by 'R' or 'W' for read or write.
- Set the preprocessor `MEMORY_USE_SWAPPING_UNIT` to `ENABLE` to enable the data swapping function in the memory controller (Currently only supports hybrid memory systems).
- Set the preprocessor `MEMORY_USE_OS_TRANSPARENT_MANAGEMENT` to `ENABLE` for enabling os transparent data management of hybrid memory systems (Currently part of paper [CAMEO](https://doi.org/10.1109/MICRO.2014.63), [MemPod](https://doi.org/10.1109/HPCA.2017.39), variable granularity, TROM (Tracking Read Only Method), and TLTOM (Tracking Load and Translation Only Method) are implemented).

You can also modify the preprocessors in the [./include/ChampSim/champsim_constants.h](include/ChampSim/champsim_constants.h) file to try different CPU configurations. For example,
- Set the preprocessor `CPU_BRANCH_PREDICTOR` to `BRANCH_USE_BIMODAL` to use the bimodal branch predictor. The other available branch predictors are `BRANCH_USE_GSHARE`, `BRANCH_USE_HASHED_PERCEPTRON`, and `BRANCH_USE_PERCEPTRON`.
- Following the same pattern, select the branch target buffer with `CPU_BRANCH_TARGET_BUFFER`; the cache prefetchers with `CPU_L1I_PREFETCHER` / `CPU_L1D_PREFETCHER` / `CPU_L2C_PREFETCHER` / `LLC_PREFETCHER`; and the cache replacement policies with `LLC_REPLACEMENT_POLICY` / `CPU_L1D_REPLACEMENT_POLICY` / `CPU_L2C_REPLACEMENT_POLICY`.

For a detailed explanation of how ChampSim and Ramulator are integrated — including the templated `MEMORY_CONTROLLER`, the runtime DRAM type resolution, address partitioning for hybrid memory, and the OS-transparent management layer — see [integration_architecture.md](integration_architecture.md).

## ChampSim
[ChampSim](https://github.com/ChampSim/ChampSim) is a trace-based simulator for a microarchitecture study. If you have questions about how to use ChampSim, we encourage you to search the threads in the [Discussions tab](https://github.com/ChampSim/ChampSim/discussions) or start your own thread. If you are aware of a bug or have a feature request, open a new [Issue](https://github.com/ChampSim/ChampSim/issues).

Because ChampSim is the result of academic research, if you use this software in your work, please cite it using the following reference:

    Gober, N., Chacon, G., Wang, L., Gratz, P. V., Jimenez, D. A., Teran, E., Pugsley, S., & Kim, J. (2022). The Championship Simulator: Architectural Simulation for Education and Competition. https://doi.org/10.48550/arXiv.2210.14324

If you use ChampSim in your work, you may submit a pull request modifying [`PUBLICATIONS_USING_CHAMPSIM.bib`](https://github.com/ChampSim/ChampSim/blob/master/PUBLICATIONS_USING_CHAMPSIM.bib) to have it featured in [the documentation](https://champsim.github.io/ChampSim/master/Publications-using-champsim.html).

## Ramulator: A DRAM Simulator
[Ramulator](https://github.com/CMU-SAFARI/ramulator) is a fast and cycle-accurate DRAM simulator \[1, 2\] that supports a wide array of commercial, as well as academic, DRAM standards:

- DDR3 (2007), DDR4 (2012)
- LPDDR3 (2012), LPDDR4 (2014)
- GDDR5 (2009)
- WIO (2011), WIO2 (2014)
- HBM (2013)
- SALP \[3\]
- TL-DRAM \[4\]
- RowClone \[5\]
- DSARP \[6\]

The initial release of Ramulator is described in the following paper:
>Y. Kim, W. Yang, O. Mutlu.
>"[**Ramulator: A Fast and Extensible DRAM Simulator**](https://people.inf.ethz.ch/omutlu/pub/ramulator_dram_simulator-ieee-cal15.pdf)".
>In _IEEE Computer Architecture Letters_, March 2015.

For information on new features, along with an extensive memory characterization using Ramulator, please read:
>S. Ghose, T. Li, N. Hajinazar, D. Senol Cali, O. Mutlu.
>"[**Demystifying Complex Workload–DRAM Interactions: An Experimental Study**](https://people.inf.ethz.ch/omutlu/pub/Workload-DRAM-Interaction-Analysis_sigmetrics19_pomacs19.pdf)".
>In _Proceedings of the ACM International Conference on Measurement and Modeling of Computer Systems (SIGMETRICS)_, June 2019 ([slides](https://people.inf.ethz.ch/omutlu/pub/Workload-DRAM-Interaction-Analysis_sigmetrics19-talk.pdf)).
>In _Proceedings of the ACM on Measurement and Analysis of Computing Systems (POMACS)_, 2019.

[\[1\] Kim et al. *Ramulator: A Fast and Extensible DRAM Simulator.* IEEE CAL
2015.](https://people.inf.ethz.ch/omutlu/pub/ramulator_dram_simulator-ieee-cal15.pdf)  
[\[2\] Ghose et al. *Demystifying Complex Workload–DRAM Interactions: An Experimental Study.* SIGMETRICS 2019.](https://people.inf.ethz.ch/omutlu/pub/Workload-DRAM-Interaction-Analysis_sigmetrics19_pomacs19.pdf)  
[\[3\] Kim et al. *A Case for Exploiting Subarray-Level Parallelism (SALP) in
DRAM.* ISCA 2012.](https://users.ece.cmu.edu/~omutlu/pub/salp-dram_isca12.pdf)  
[\[4\] Lee et al. *Tiered-Latency DRAM: A Low Latency and Low Cost DRAM
Architecture.* HPCA 2013.](https://users.ece.cmu.edu/~omutlu/pub/tldram_hpca13.pdf)  
[\[5\] Seshadri et al. *RowClone: Fast and Energy-Efficient In-DRAM Bulk Data
Copy and Initialization.* MICRO
2013.](https://users.ece.cmu.edu/~omutlu/pub/rowclone_micro13.pdf)  
[\[6\] Chang et al. *Improving DRAM Performance by Parallelizing Refreshes with
Accesses.* HPCA 2014.](https://users.ece.cmu.edu/~omutlu/pub/dram-access-refresh-parallelization_hpca14.pdf)

## Ramulator V2.0
[Ramulator 2.0](https://github.com/CMU-SAFARI/ramulator2) is a modern, modular, and extensible cycle-accurate DRAM simulator. It is the successor of Ramulator 1.0 [Kim+, CAL'16], achieving both fast simulation speed and ease of extension. The goal of Ramulator 2.0 is to enable rapid and agile implementation and evaluation of design changes in the memory controller and DRAM to meet the increasing research effort in improving the performance, security, and reliability of memory systems. Ramulator 2.0 abstracts and models key components in a DRAM-based memory system and their interactions into shared interfaces and independent implementations, enabling easy modification and extension of the modeled functions of the memory controller and DRAM. 

This Github repository contains the public version of Ramulator 2.0. From time to time, we will synchronize improvements of the code framework, additional functionalities, bug fixes, etc. from our internal version. Ramulator 2.0 is in its early stage and welcomes your contribution as well as new ideas and implementations in the memory system!

Currently, Ramulator 2.0 provides the DRAM models for the following standards:
- DDR3, DDR4, DDR5
- LPDDR5
- GDDR6
- HBM(2), HBM3

Ramulator 2.0 also provides implementations for the following RowHammer mitigation techniques:
- PARA [[Kim+, ISCA'14]](https://people.inf.ethz.ch/omutlu/pub/dram-row-hammer_isca14.pdf)
- TWiCe [[Lee+, ISCA'19]](https://ieeexplore.ieee.org/document/8980327)
- Graphene [[Park+, MICRO'20]](https://microarch.org/micro53/papers/738300a001.pdf)
- BlockHammer [[Yağlıkçı+, HPCA'21]](https://people.inf.ethz.ch/omutlu/pub/BlockHammer_preventing-DRAM-rowhammer-at-low-cost_hpca21.pdf)
- Hydra [[Qureshi+, ISCA'22]](https://memlab.ece.gatech.edu/papers/ISCA_2022_1.pdf)
- Randomized Row Swap (RRS) [[Saileshwar+, ASPLOS'22]](https://gururaj-s.github.io/assets/pdf/ASPLOS22_Saileshwar.pdf)
- AQUA [[Saxena+, MICRO'22]](https://memlab.ece.gatech.edu/papers/MICRO_2022_1.pdf)
- An "Oracle" Refresh Mitigation [[Kim+, ISCA'20]](https://people.inf.ethz.ch/omutlu/pub/Revisiting-RowHammer_isca20-FINAL-DO-NOT_DISTRIBUTE.pdf)

# Download dependencies

ChampSim uses [vcpkg](https://vcpkg.io) to manage its dependencies. In this repository, vcpkg is included as a submodule. You can download the dependencies with
```sh
$ git submodule update --init
$ ./external/vcpkg/bootstrap-vcpkg.sh
$ ./external/vcpkg/vcpkg install
```

# Download DPC-3 trace

Traces used for the 3rd Data Prefetching Championship (DPC-3) can be found here. (https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/) A set of traces used for the 2nd Cache Replacement Championship (CRC-2) can be found from this link. (http://bit.ly/2t2nkUj)

Storage for these traces is kindly provided by Daniel Jimenez (Texas A&M University) and Mike Ferdman (Stony Brook University). If you find yourself frequently using ChampSim, it is highly encouraged that you maintain your own repository of traces, in case the links ever break.

You can use the helper script to download DPC-3 traces (the default `dpc3_max_simpoint.txt` set is ~20GB, max simpoint only):
```sh
$ cd scripts
$ ./download_dpc3_traces.sh                          # default: dpc3_max_simpoint.txt
$ ./download_dpc3_traces.sh dpc3_max_simpoint2.txt   # or pass any trace-list file
```
The script reads a list of trace filenames (one per line), defaults to `dpc3_max_simpoint.txt`, and downloads any missing traces (via `wget -c`) into the `dpc3_traces/` directory inside `ChampSim-Ramulator/`.

# Build and debug

Before starting to build or debug this project, you might need to be familiar with the [Visual Studio Code tutorial](https://code.visualstudio.com/docs/cpp/config-linux). Also, a C++20 compiler is required for compilation (Ramulator 2.0 uses C++20 features such as `concept`, `consteval`, and `.contains()`).

## Build
Build methods are explained below.

### 1. Visual Studio Code-based method.
- You may need to modify the compiler's path in the [tasks.json](.vscode/tasks.json) file in the `.vscode` directory.
- Click `Run Build Task` in the `Terminal` tab.

If your Visual Studio Code does not show valid tasks to run, reloading your window might solve the problem.

#### CMake + ccache tasks (recommended for faster builds)
Use [CMake](https://cmake.org/) (≥ 3.21) to enable parallel builds and [ccache](https://ccache.dev/) to speed up recompilation. Build configurations are selected via [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) — the available presets (`default`, `debug`, `release`, `strict`) and their flag combinations are described in [`CMakePresets.json`](CMakePresets.json) (each preset's `displayName` and `description` fields). Run the `CMake: List presets` task (or `cmake --list-presets=all` from a terminal) to enumerate every preset.

The following tasks are available in `Terminal → Run Task`:

| Task | Description |
|---|---|
| `CMake: Build (preset)` | Prompts for a preset, then configures and builds with ccache + `-j8`. Output: `bin/champsim_plus_ramulator`. |
| `CMake: List presets` | Runs `cmake --list-presets=all` in the workspace, printing every configure / build / test / package / workflow preset from `CMakePresets.json`. |
| `CMake: Clean build artifacts` | Removes compiled objects and the binary while keeping `build/` configured (next build is still cache-fast). |
| `CMake: Wipe build directory` | Deletes the entire `build/` directory. Use when switching toolchains or after vcpkg dependency changes. |
| `ccache: Show statistics` | Prints cache hit/miss counters and cache size for this project. |
| `ccache: Show configuration` | Prints the active ccache configuration, including the project-local cache dir (`.ccache/`). |
| `ccache: Clear cache` | Wipes the project-local ccache. The next build will be a full cold rebuild. |

The project-local ccache directory is `ChampSim-Ramulator/.ccache/` (isolated from any global user cache).

### 2. Command line-based method.
By referring to the contents of [tasks.json](.vscode/tasks.json) file in the `.vscode` directory, input the command like below,
```
$ [COMPILER] -O3 -g -Wall -fopenmp -std=c++20 -I [VCPKG_INCLUDE_PATH] -I [PROJECT_INCLUDE_PATH] -L [VCPKG_LIBRARY_PATH] -L [VCPKG_MANUAL_LINK_LIBRARY_PATH]
[CHAMPSIM_MAIN_SOURCE_PATH] [CHAMPSIM_BRANCH_SOURCE_PATH] [CHAMPSIM_PREFETCHER_SOURCE_PATH] [CHAMPSIM_REPLACEMENT_SOURCE_PATH] [CHAMPSIM_BRANCH_TARGET_BUFFER_SOURCE_PATH] [RAMULATOR_SOURCE_PATH] [RAMULATOR2_SOURCE_PATH] [PROJECT_SOURCE_PATH]
-l lzma -l z -l bz2 -l spdlog -l yaml-cpp -l fmt
-o project_directory/bin/champsim_plus_ramulator
```
where [COMPILER] is the compiler's name, such as g++. Each `[..._SOURCE_PATH]` token expands to the source globs defined in the [tasks.json](.vscode/tasks.json) file.

## Debug
Debug methods are explained below.

### 1. Visual Studio Code-based method.
- You may need to modify the debugger's path in the [launch.json](.vscode/launch.json) file in the `.vscode` directory.
- Click `Start Debugging` in the `Run` tab.

If your Visual Studio Code does not show valid tasks to run, reloading your window might solve the problem.

### 2. Command line-based method.
By referring to the contents of [launch.json](.vscode/launch.json) file in the `.vscode` directory, input the below command,
```
(waiting for update)
```

# Run simulation
According to the setting in the `ProjectConfiguration.h` file, the input parameters vary. There are four types of input patterns.
## 1. ChampSim + Ramulator 1.0 with hybrid memory systems
If the preprocessor `RAMULATOR` is `ENABLE` and `MEMORY_USE_HYBRID` is `ENABLE`, execute the binary as follows,
```
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] [CFG1] [CFG2] [TRACE]
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] --stats [FILENAME] [CFG1] [CFG2] [TRACE]
```
where [EXECUTION] is the executable file's name, such as ./bin/champsim_plus_ramulator, [N_WARM] is the number of instructions for warmup (1 million), [N_SIM] is the number of instructions for detailed simulation (10 million),
[CFG1] and [CFG2] are the memory configuration files' names (configs/HBM-config.cfg), [TRACE] is the trace name (619.lbm_s-4268B.champsimtrace.xz), and [FILENAME] is the statistics file name. For example,
```
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 path_to_configs/HBM-config.cfg path_to_configs/DDR4-config.cfg path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. Statistics written to HBM_DDR4.stats
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 --stats my_output.txt path_to_configs/HBM-config.cfg path_to_configs/DDR4-config.cfg path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. Statistics written to my_output.txt
# NOTE: optional --stats flag changes the statistics output filename
```

## 2. ChampSim + Ramulator 1.0 with single memory systems
If the preprocessor `RAMULATOR` is `ENABLE` and `MEMORY_USE_HYBRID` is `DISABLE`, execute the binary as follows,
```
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] [CFG1] [TRACE]
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] --stats [FILENAME] [CFG1] [TRACE]
```
where [EXECUTION] is the executable file's name, such as ./bin/champsim_plus_ramulator, [N_WARM] is the number of instructions for warmup (1 million), [N_SIM] is the number of instructions for detailed simulation (10 million),
[CFG1] is the memory configuration file's name (configs/HBM-config.cfg), [TRACE] is the trace name (619.lbm_s-4268B.champsimtrace.xz), and [FILENAME] is the statistics file name. For example,
```
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 path_to_configs/HBM-config.cfg path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. Statistics written to HBM.stats
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 --stats my_output.txt path_to_configs/HBM-config.cfg path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. Statistics written to my_output.txt
# NOTE: optional --stats flag changes the statistics output filename
```

## 3. ChampSim + Ramulator 2.0 with hybrid memory systems
If the preprocessor `RAMULATOR2` is `ENABLE` and `MEMORY_USE_HYBRID` is `ENABLE`, execute the binary as follows,
```
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] [YAML1] [YAML2] [TRACE]
```
where [EXECUTION] is the executable file's name, such as ./bin/champsim_plus_ramulator, [N_WARM] is the number of instructions for warmup (1 million), [N_SIM] is the number of instructions for detailed simulation (10 million),
[YAML1] and [YAML2] are the Ramulator 2.0 configuration files' names for the fast tier and the slow tier respectively (configs/r2/HBM.yaml configs/r2/DDR4.yaml), and [TRACE] is the trace name (619.lbm_s-4268B.champsimtrace.xz). The first YAML configures the fast memory (`MEMORY_NUMBER_ONE`) and the second configures the slow memory (`MEMORY_NUMBER_TWO`); the two tiers share a single global address space. Enabling `MEMORY_USE_HYBRID` automatically enables the data-swapping unit (`MEMORY_USE_SWAPPING_UNIT`) and the OS-transparent management layer (`MEMORY_USE_OS_TRANSPARENT_MANAGEMENT`). For example,
```
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 configs/r2/HBM.yaml configs/r2/DDR4.yaml path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. YAML configs: configs/r2/HBM.yaml, configs/r2/DDR4.yaml
```
Any two of the Ramulator 2.0 device configs under `configs/r2/` (DDR3, DDR4, DDR5, GDDR6, HBM, HBM2, HBM3, LPDDR5) can be paired; there are no dedicated fast/slow config files, so pass a smaller/faster device first and a larger/slower device second.

## 4. ChampSim + Ramulator 2.0 with single memory systems
If the preprocessor `RAMULATOR2` is `ENABLE` (and `MEMORY_USE_HYBRID` is `DISABLE`), execute the binary as follows,
```
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] [YAML] [TRACE]
```
where [EXECUTION] is the executable file's name, such as ./bin/champsim_plus_ramulator, [N_WARM] is the number of instructions for warmup (1 million), [N_SIM] is the number of instructions for detailed simulation (10 million),
[YAML] is the Ramulator 2.0 configuration file's name (configs/r2/DDR4.yaml), and [TRACE] is the trace name (619.lbm_s-4268B.champsimtrace.xz). For example,
```
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 configs/r2/DDR4.yaml path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done. YAML config: configs/r2/DDR4.yaml
```
Ramulator 2.0 YAML configs are provided for DDR3, DDR4, DDR5, GDDR6, HBM, HBM2, HBM3, and LPDDR5 under `configs/r2/`.

## 5. ChampSim with single memory systems
If the preprocessor `RAMULATOR` is `DISABLE`, `RAMULATOR2` is `DISABLE`, and `MEMORY_USE_HYBRID` is `DISABLE`, execute the binary as follows,
```
$ [EXECUTION] --warmup-instructions [N_WARM] --simulation-instructions [N_SIM] [TRACE]
```
where [EXECUTION] is the executable file's name, such as ./bin/champsim_plus_ramulator, [N_WARM] is the number of instructions for warmup (1 million), [N_SIM] is the number of instructions for detailed simulation (10 million), [TRACE] is the trace name (619.lbm_s-4268B.champsimtrace.xz). For example,
```
$ ./bin/champsim_plus_ramulator --warmup-instructions 1000000 --simulation-instructions 2000000 path_to_traces/619.lbm_s-4268B.champsimtrace.xz
Simulation done.
```

The number of warmup and simulation instructions given will be the number of instructions retired. Note that the statistics printed at the end of the simulation include only the simulation phase.

# How to create traces

Program traces are available in various locations; however, many ChampSim users prefer to trace their own programs for research purposes.
Example tracing utilities are provided in the `tracer/` directory.

# Evaluate Simulation

ChampSim measures IPC (Instructions Per Cycle) as a performance metric. <br>
Some other useful metrics are printed at the end of the simulation. <br>

# Test

## End-to-end tests
End-to-end testing is implemented in [test/end_to_end/](test/end_to_end/). For details, please read [test/end_to_end/README.md](test/end_to_end/README.md).

# Miscellaneous

If IntelliSense still doesn't work properly, it might be because of the version of the C++ language standard used. To solve this problem, you need to open Visual Studio Code, click `View` -> `Command Palette`, and in the center where a terminal is popped out, input or select `C/C++: Edit Configurations (UI)`. A new file called `c_cpp_properties.json` should be created, and its UI is opened. After you modify `C++ standard` to `c++20` (or higher) in that file. IntelliSense should work properly now.

We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) for code formatting of this project. To configure the path of the clang-format executable, please refer to the [settings.json](.vscode/settings.json). A simple tutorial for using clang-format is at [here](https://code.visualstudio.com/docs/cpp/cpp-ide#_code-formatting).
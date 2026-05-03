<p align="center">
  <h1 align="center"> ChampSim + Ramulator </h1>
</p>

# About This Project
This project is based on the [ChampSim](https://github.com/yousei-github/ChampSim) (Commit: [24cc41b](https://github.com/ChampSim/ChampSim/commit/24cc41bb94bac4da7ffa18774524529bb0e57c44)) and [Ramulator](https://github.com/yousei-github/ramulator) (Commit: [743b940](https://github.com/CMU-SAFARI/ramulator/commit/743b940b70a8e18bcffb14eec22d2ed731059540)), and it can be modified to simulate hybrid memory systems. You can modify the preprocessors in the `./inc/ProjectConfiguration.h` file and recompile this project to try different functionalities. For example,
- Set the preprocessor `RAMULATOR` to `ENABLE` for enabling Ramulator or to `DISABLE` for just using ChampSim.
- Set the preprocessor `MEMORY_USE_HYBRID` to `ENABLE` for enabling hybrid memory systems or to `DISABLE` for enabling single memory systems.
- Set the preprocessor `CPU_USE_MULTIPLE_CORES` to `ENABLE` for enabling multiple cores to run the simulation. Note that you also need to add multiple trace paths to run this simulator.
- Set the preprocessor `PRINT_STATISTICS_INTO_FILE` to `ENABLE` for printing statistics into the `.statistics` file.
- Set the preprocessor `PRINT_MEMORY_TRACE` to `ENABLE` for printing memory trace into the `.trace` file. Each line in the trace file represents a memory request, with the hexadecimal address followed by 'R' or 'W' for read or write.
- Set the preprocessor `MEMORY_USE_SWAPPING_UNIT` to `ENABLE` for enabling the data swapping function in the memory controller (Currently only supports hybrid memory systems).
- Set the preprocessor `MEMORY_USE_OS_TRANSPARENT_MANAGEMENT` to `ENABLE` for enabling os transparent data management of hybrid memory systems (Currently part of paper [CAMEO](https://doi.org/10.1109/MICRO.2014.63), [MemPod](https://doi.org/10.1109/HPCA.2017.39), variable granularity, TROM (Tracking Read Only Method), and TLTOM (Tracking Load and Translation Only Method) are implemented).
- Set the preprocessor `BRANCH_PREDICTOR` to `BRANCH_USE_BIMODAL` for using the bimodal branch predictor. Similarly, there are gshare, hashed_perceptron, and perceptron branch predictors. Following this logic, you can also modify other preprocessors, such as `INSTRUCTION_PREFETCHER`, `LLC_REPLACEMENT_POLICY`, `LLC_PREFETCHER`, and so on.

The CPU's parameters are defined in the `./inc/ChampSim/champsim_constants.h` file.

For a detailed explanation of how ChampSim and Ramulator are integrated — including the templated `MEMORY_CONTROLLER`, the runtime DRAM type resolution, address partitioning for hybrid memory, and the OS-transparent management layer — see [integration_architecture.md](integration_architecture.md).

## ChampSim
[ChampSim](https://github.com/ChampSim/ChampSim) is a trace-based simulator for a microarchitecture study. If you have questions about how to use ChampSim, we encourage you to search the threads in the [Discussions tab](https://github.com/ChampSim/ChampSim/discussions) or start your own thread. If you are aware of a bug or have a feature request, open a new [Issue](https://github.com/ChampSim/ChampSim/issues).

Because ChampSim is the result of academic research, if you use this software in your work, please cite it using the following reference:

    Gober, N., Chacon, G., Wang, L., Gratz, P. V., Jimenez, D. A., Teran, E., Pugsley, S., & Kim, J. (2022). The Championship Simulator: Architectural Simulation for Education and Competition. https://doi.org/10.48550/arXiv.2210.14324

If you use ChampSim in your work, you may submit a pull request modifying [`PUBLICATIONS_USING_CHAMPSIM.bib`](https://github.com/ChampSim/ChampSim/blob/master/PUBLICATIONS_USING_CHAMPSIM.bib) to have it featured in [the documentation](https://champsim.github.io/ChampSim/master/Publications-using-champsim.html).

Traces for the 3rd Data Prefetching Championship (DPC-3) can be found from here (https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/). A set of traces used for the 2nd Cache Replacement Championship (CRC-2) can be found from this link. (http://bit.ly/2t2nkUj)

Storage for these traces is kindly provided by Daniel Jimenez (Texas A&M University) and Mike Ferdman (Stony Brook University). If you frequently use ChampSim, it is highly encouraged that you maintain your own repository of traces in case the links ever break.

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

# Download dependencies

ChampSim uses [vcpkg](https://vcpkg.io) to manage its dependencies. In this repository, vcpkg is included as a submodule. You can download the dependencies with
```sh
$ git submodule update --init
$ ./vcpkg/bootstrap-vcpkg.sh
$ ./vcpkg/vcpkg install
```

# Download DPC-3 trace

Professor Daniel Jimenez at Texas A&M University kindly provided traces for DPC-3. Use the following script to download these traces (~20GB in size and max simpoint only).
```sh
$ cd scripts
$ ./download_dpc3_traces.sh
```

# Build and debug

Before starting to build or debug this project, you might need to be familiar with [Visual Studio Code tutorial](https://code.visualstudio.com/docs/cpp/config-linux). Also, a C++11 compiler is required for compilation.

## Build
Build methods are explained below.

### 1. Visual Studio Code-based method.
- You may need to modify the compiler's path in the [tasks.json](.vscode/tasks.json) file in the `.vscode` directory.
- Click `Run Build Task` in the `Terminal` tab.

If your Visual Studio Code does not show valid tasks to run, reloading your window might solve the problem.

#### CMake + ccache tasks (recommended for faster builds)
Use [CMake](https://cmake.org/) to enable parallel builds and [ccache](https://ccache.dev/) to speed up recompilation. The following tasks are available in `Terminal → Run Task`:

| Task | Description |
|---|---|
| `CMake: Build (ccache, parallel)` | Incremental per-file build using ccache with `-j8`. Output: `bin/champsim_plus_ramulator`. Configure runs automatically on first use or after a wipe. |
| `CMake: Clean build artifacts` | Removes compiled objects and the binary while keeping `build/` configured (next build is still cache-fast). |
| `CMake: Wipe build directory` | Deletes the entire `build/` directory. Use when switching toolchains or after vcpkg dependency changes. |
| `ccache: Show statistics` | Prints cache hit/miss counters and cache size for this project. |
| `ccache: Show configuration` | Prints the active ccache configuration, including the project-local cache dir (`.ccache/`). |
| `ccache: Clear cache` | Wipes the project-local ccache. The next build will be a full cold rebuild. |

The project-local ccache directory is `ChampSim-Ramulator/.ccache/` (isolated from any global user cache).

### 2. Command line-based method.
By referring to the contents of [tasks.json](.vscode/tasks.json) file in the `.vscode` directory, input the command like below,
```
$ [COMPILER] -O3 -g -Wall -fopenmp -std=c++17 -I [VCPKG_INCLUDE_PATH] -I [PROJECT_INCLUDE_PATH] -L [VCPKG_LIBRARY_PATH] -L [VCPKG_MANUAL_LINK_LIBRARY_PATH]
[CHAMPSIM_MAIN_SOURCE_PATH] [CHAMPSIM_BRANCH_SOURCE_PATH] [CHAMPSIM_PREFETCHER_SOURCE_PATH] [CHAMPSIM_REPLACEMENT_SOURCE_PATH] [CHAMPSIM_BRANCH_TARGET_BUFFER_SOURCE_PATH] [RAMULATOR_SOURCE_PATH] [PROJECT_SOURCE_PATH]
-l lzma -l z -l bz2 -l fmt
-o project_directory/bin/champsim_plus_ramulator
```
where [COMPILER] is the compiler's name, such as g++.

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
According to the setting in the `ProjectConfiguration.h` file, the input parameters vary. There are three types of input patterns.
## 1. ChampSim + Ramulator with hybrid memory systems
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

## 2. ChampSim + Ramulator with single memory systems
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

## 3. ChampSim with single memory systems
If the preprocessor `RAMULATOR` is `DISABLE` and `MEMORY_USE_HYBRID` is `DISABLE`, execute the binary as follows,
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

# Miscellaneous

If IntelliSense still doesn't work properly, it might be because of the version of the C++ language standard used. To solve this problem, you need to open Visual Studio Code, click `View` -> `Command Palette`, and in the center where a terminal is popped out, input or select `C/C++: Edit Configurations (UI)`. A new file called `c_cpp_properties.json` should be created, and its UI is opened. After you modify `C++ standard` to `gnu++17` (or higher) in that file. IntelliSense should work properly now.

We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) for code formatting of this project. To configure the path of the clang-format executable, please refer to the [settings.json](.vscode/settings.json). A simple tutorial for using clang-format is at [here](https://code.visualstudio.com/docs/cpp/cpp-ide#_code-formatting).
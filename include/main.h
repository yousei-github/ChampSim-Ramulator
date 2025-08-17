#ifndef __MAIN_H
#define __MAIN_H

/* Header */

// Includes for this project
#include "ProjectConfiguration.h" // User file

// Includes of ChampSim
#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>

#include <CLI/CLI.hpp>
#endif /* USE_VCPKG */

#include <algorithm>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

#include "ChampSim/cache.h" // for CACHE
#include "ChampSim/champsim.h"

#if (USER_CODES == ENABLE)
#ifndef CHAMPSIM_TEST_BUILD
#include "ChampSim/core_inst.h"
#endif

#else
#ifndef CHAMPSIM_TEST_BUILD
#include "ChampSim/core_inst.inc"
#endif

#endif /* USER_CODES */

#include "ChampSim/defaults.hpp"
#include "ChampSim/dram_controller.h"
#include "ChampSim/environment.h"
#include "ChampSim/ooo_cpu.h" // for O3_CPU
#include "ChampSim/phase_info.h"
#include "ChampSim/stats_printer.h"
#include "ChampSim/tracereader.h"
#include "ChampSim/vmem.h"

// Includes of Ramulator
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

/// Standards of Ramulator
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

/* Macro */

/* Type */

/* Prototype */

/* Variable */

/* Function */

#endif

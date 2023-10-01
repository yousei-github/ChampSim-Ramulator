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

#if (USER_CODES == ENABLE)
// #include "ChampSim/ptw.h"
#endif

/* Macro */

/* Type */

/* Prototype */

/* Variable */

/* Function */

#endif

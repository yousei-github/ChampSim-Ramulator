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

#ifndef CHAMPSIM_H
#define CHAMPSIM_H

#include <cstdint>
#include <exception>
#include <vector>

#include "ChampSim/environment.h"
#include "ChampSim/phase_info.h"
#include "ChampSim/tracereader.h"
#include "ProjectConfiguration.h" // User file

namespace champsim
{
struct deadlock : public std::exception
{
    const uint32_t which;

    explicit deadlock(uint32_t cpu): which(cpu) {}
};

#ifdef DEBUG_PRINT
constexpr bool debug_print = true;
#else
constexpr bool debug_print = false;
#endif

#if (USER_CODES == ENABLE)
std::vector<phase_stats> main(environment& env, std::vector<phase_info>& phases, std::vector<tracereader>& traces);
#endif // USER_CODES

} // namespace champsim

#endif

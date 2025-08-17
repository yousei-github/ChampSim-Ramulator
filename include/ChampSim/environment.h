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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <functional>
#include <vector>

#include "ChampSim/cache.h"
#include "ChampSim/dram_controller.h"
#include "ChampSim/ooo_cpu.h"
#include "ChampSim/operable.h"
#include "ChampSim/ptw.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include <forward_list>

#include "ChampSim/chrono.h"
#include "ChampSim/defaults.hpp"
#include "ChampSim/vmem.h"

namespace champsim
{

struct environment
{
    virtual std::vector<std::reference_wrapper<O3_CPU> > cpu_view()          = 0;
    virtual std::vector<std::reference_wrapper<CACHE> > cache_view()         = 0;
    virtual std::vector<std::reference_wrapper<PageTableWalker> > ptw_view() = 0;

#if (RAMULATOR == ENABLE)
#else
    virtual MEMORY_CONTROLLER& dram_view() = 0;
#endif /* RAMULATOR */

    virtual std::vector<std::reference_wrapper<operable> > operable_view() = 0;
};

namespace configured
{

#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
template<unsigned long long ID, typename MEMORY_TYPE, typename MEMORY_TYPE2>
struct generated_environment;

#else
template<unsigned long long ID, typename MEMORY_TYPE>
struct generated_environment;

#endif /* MEMORY_USE_HYBRID */
#else
template<unsigned long long ID>
struct generated_environment;

#endif /* RAMULATOR */

template<typename R, typename... PTWs>
auto build(PTWs... builders)
{
    std::forward_list<R> retval {};
    (..., retval.emplace_front(builders));
    return retval;
}

} // namespace configured

} // namespace champsim

#else
/* Original code of ChampSim */

namespace champsim
{

struct environment
{
    virtual std::vector<std::reference_wrapper<O3_CPU> > cpu_view()          = 0;
    virtual std::vector<std::reference_wrapper<CACHE> > cache_view()         = 0;
    virtual std::vector<std::reference_wrapper<PageTableWalker> > ptw_view() = 0;
    virtual MEMORY_CONTROLLER& dram_view()                                   = 0;
    virtual std::vector<std::reference_wrapper<operable> > operable_view()   = 0;
};

namespace configured
{
template<unsigned long long ID>
struct generated_environment;
} // namespace configured

} // namespace champsim

#endif /* USER_CODES */

#endif

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers): generated magic numbers

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
/* No code here */

#else
#include <forward_list>

#include "ChampSim/core_inst.inc"
#include "ChampSim/environment.h"

#if __has_include("legacy_bridge.h")
#include "legacy_bridge.h"
#endif

#include "ChampSim/chrono.h"
#include "ChampSim/defaults.hpp"
#include "ChampSim/vmem.h"

namespace champsim::configured
{

template<typename R, typename... PTWs>
auto build(PTWs... builders)
{
    std::forward_list<R> retval {};
    (..., retval.emplace_front(builders));
    return retval;
}

} // namespace champsim::configured

#if __has_include("core_inst.cc.inc")
#include "core_inst.cc.inc"
#endif

#endif /* USER_CODES */

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

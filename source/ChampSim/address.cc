#include "ChampSim/address.h"

#include "ChampSim/champsim.h"
#include "ChampSim/util/to_underlying.h"
#include "ChampSim/util/units.h"

auto champsim::lowest_address_for_size(champsim::data::bytes sz) -> champsim::address
{
    return champsim::address {static_cast<champsim::address::underlying_type>(sz.count())};
}

auto champsim::lowest_address_for_width(champsim::data::bits width) -> champsim::address
{
    return champsim::lowest_address_for_size(champsim::data::bytes {1LL << champsim::to_underlying(width)});
}

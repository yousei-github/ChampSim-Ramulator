#ifndef RAMULATOR_CONTROLLER_IBLOCKHAMMER_H
#define RAMULATOR_CONTROLLER_IBLOCKHAMMER_H

#include "Ramulator2/base/base.h"

namespace Ramulator {

class IBlockHammer {
public:
    virtual bool is_act_safe(Request& req) = 0;
};

}       //  namespace Ramulator

#endif  //  RAMULATOR_CONTROLLER_IBLOCKHAMMER_H
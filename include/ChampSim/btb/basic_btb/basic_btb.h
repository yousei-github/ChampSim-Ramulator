#ifndef BTB_BASIC_BTB_H
#define BTB_BASIC_BTB_H

#include "ChampSim/address.h"
#include "ChampSim/btb/basic_btb/direct_predictor.h"
#include "ChampSim/btb/basic_btb/indirect_predictor.h"
#include "ChampSim/btb/basic_btb/return_stack.h"
#include "ChampSim/modules.h"

class basic_btb : champsim::modules::btb
{
    return_stack ras {};
    indirect_predictor indirect {};
    direct_predictor direct {};

public:
    using btb::btb;

    basic_btb(): btb(nullptr) {}

    // void initialize_btb();
    std::pair<champsim::address, bool> btb_prediction(champsim::address ip);
    void update_btb(champsim::address ip, champsim::address branch_target, bool taken, uint8_t branch_type);
};

#endif

#include <filesystem>
#include <fstream>
#include <iostream>

#include "Ramulator2/base/exception.h"
#include "Ramulator2/frontend/frontend.h"

namespace Ramulator
{

#if (USER_CODES == ENABLE)

class CHAMPSIM : public IFrontEnd, public Implementation
{
    RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, CHAMPSIM, "ChampSim", "ChampSim frontend.")

    int m_num_cores = -1;

public:
    void init() override {};

    int get_num_cores() override { return m_num_cores; };

    void set_num_cores(int num_cores) override { m_num_cores = num_cores; };

private:
    // ChampSim frontend doesn't need to tick,
    // ChampSim frontend is used to complete the Ramulator::IMemorySystem's initialization (connect_frontend)
    void tick() override {};

    bool receive_external_requests(int req_type_id, Addr_t addr, int source_id, std::function<void(Request&)> callback) override { return true; };

    bool is_finished() override { return true; };
};

#endif /* USER_CODES */

} // namespace Ramulator
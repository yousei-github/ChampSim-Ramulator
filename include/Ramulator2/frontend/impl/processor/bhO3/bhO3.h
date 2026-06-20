#include <functional>
#include <limits>

#include "ProjectConfiguration.h" // User file

#include "Ramulator2/base/utils.h"
#include "Ramulator2/frontend/frontend.h"
#include "Ramulator2/translation/translation.h"
#include "Ramulator2/frontend/impl/processor/bhO3/bhcore.h"
#include "Ramulator2/frontend/impl/processor/bhO3/bhllc.h"

namespace Ramulator {

class BHO3 final : public IFrontEnd, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, BHO3, "BHO3", "Simple timing model OoO processor frontend.")

  private:
    ITranslation*  m_translation;

    int m_num_cores = -1;
    int m_num_blocking_cores = -1;
    std::vector<BHO3Core*> m_cores;
    BHO3LLC* m_llc;

    size_t m_num_expected_insts = 0;
    uint64_t m_num_max_cycles = 0;

    bool llc_serialize = false;
    std::string llc_serialization_filename;
    bool llc_deserialize = false;
    std::string llc_deserialization_filename;

  public:
    void init() override;
    void tick() override;
    void receive(Request& req);
    bool is_finished() override;
    void connect_memory_system(IMemorySystem* memory_system) override;
    int get_num_cores() override;
    BHO3LLC* get_llc();
    std::vector<BHO3Core*>& get_cores();
};

}        // namespace Ramulator
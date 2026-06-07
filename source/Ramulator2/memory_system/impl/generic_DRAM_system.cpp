#include "memory_system/memory_system.h"
#include "translation/translation.h"
#include "dram_controller/controller.h"
#include "addr_mapper/addr_mapper.h"
#include "dram/dram.h"

namespace Ramulator {

class GenericDRAMSystem final : public IMemorySystem, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IMemorySystem, GenericDRAMSystem, "GenericDRAM", "A generic DRAM-based memory system.");

  protected:
    Clk_t m_clk = 0;
    IDRAM*  m_dram;
    IAddrMapper*  m_addr_mapper;
    std::vector<IDRAMController*> m_controllers;

  public:
    int s_num_read_requests = 0;
    int s_num_write_requests = 0;
    int s_num_other_requests = 0;


  public:
    void init() override { 
      // Create device (a top-level node wrapping all channel nodes)
      m_dram = create_child_ifce<IDRAM>();
      m_addr_mapper = create_child_ifce<IAddrMapper>();

      int num_channels = m_dram->get_level_size("channel");   

      // Create memory controllers
      for (int i = 0; i < num_channels; i++) {
        IDRAMController* controller = create_child_ifce<IDRAMController>();
        controller->m_impl->set_id(fmt::format("Channel {}", i));
        controller->m_channel_id = i;
        m_controllers.push_back(controller);
      }

      m_clock_ratio = param<uint>("clock_ratio").required();

      register_stat(m_clk).name("memory_system_cycles");
      register_stat(s_num_read_requests).name("total_num_read_requests");
      register_stat(s_num_write_requests).name("total_num_write_requests");
      register_stat(s_num_other_requests).name("total_num_other_requests");
    };

    void setup(IFrontEnd* frontend, IMemorySystem* memory_system) override { }

    bool send(Request req) override {
      m_addr_mapper->apply(req);
      int channel_id = req.addr_vec[0];
      bool is_success = m_controllers[channel_id]->send(req);

      if (is_success) {
        switch (req.type_id) {
          case Request::Type::Read: {
            s_num_read_requests++;
            break;
          }
          case Request::Type::Write: {
            s_num_write_requests++;
            break;
          }
          default: {
            s_num_other_requests++;
            break;
          }
        }
      }

      return is_success;
    };
    
    void tick() override {
      m_clk++;
      m_dram->tick();
      for (auto controller : m_controllers) {
        controller->tick();
      }
    };

    float get_tCK() override {
      return m_dram->m_timing_vals("tCK_ps") / 1000.0f;
    }

    // const SpecDef& get_supported_requests() override {
    //   return m_dram->m_requests;
    // };

#if (USER_CODES == ENABLE)
    size_t get_capacity() const override
    {
        /** @note
         * As an example of the internal organization of a DRAM chip,
         * Let's consider [DDR4 4Gb x8]:
         * 
         * - DDR4: The generation of memory technology.
         * - 4Gb: The density (capacity) of a single memory chip in gigabits.
         *   Note that the 'b' is lowercase, meaning each individual memory chip stores 4 gigabits (512 MB) of data.
         * - x8: The data bus width of that individual chip.
         *   It means the chip sends or receives data in 8-bit chunks (at a time).
         * 
         * Because your computer's CPU and memory controller (usually) expect data in a 64-bit wide block (a "rank"),
         * the memory module must combine multiple individual chips to create that data path.
         * 
         * Note that DQ stands for Data Queue (sometimes called Data I/O pins),
         * and the DQ width is simply how many data bits a chip can transfer in parallel per clock cycle.
         * This is the same as the bus width (the "x" number).
         */
        const int density         = m_dram->m_organization.density;
        const int number_of_chips = m_dram->m_channel_width / m_dram->m_organization.dq;
        const size_t capacity     = ((density * number_of_chips) / 8) * MiB; // Unit is byte
        return capacity;
    };

    int get_channel() const override
    {
        constexpr std::string_view channel_name {"channel"};
        const bool has_channel = m_dram->m_levels.contains(channel_name);

        if (! has_channel)
        {
            return 0;
        }

        const int channel_index = m_dram->m_levels(channel_name);
        const int channel       = m_dram->m_organization.count[channel_index];
        return channel;
    };

    int get_channel_width() const override
    {
        return m_dram->m_channel_width; // Unit is bit
    };

    int get_rate() const override
    {
        constexpr std::string_view rate_name {"rate"};
        const bool has_rate = m_dram->m_timings.contains(rate_name);

        if (! has_rate)
        {
            return 0;
        }

        const int rate_index = m_dram->m_timings(rate_name);
        const int rate       = m_dram->m_timing_vals(rate_index);
        return rate; // Unit is MT/s
    };

    size_t get_queue_occupancy(Request& req) const override
    {
        const int type = req.type_id;
        m_addr_mapper->apply(req);
        const int channel_id   = req.addr_vec[0];

        const size_t occupancy = m_controllers[channel_id]->get_queue_occupancy(type);

        return occupancy;
    };

    size_t get_queue_size(Request& req) const override
    {
        const int type = req.type_id;
        m_addr_mapper->apply(req);
        const int channel_id = req.addr_vec[0];

        const size_t size    = m_controllers[channel_id]->get_queue_size(type);

        return size;
    };
#endif /* USER_CODES */
};

} // namespace Ramulator

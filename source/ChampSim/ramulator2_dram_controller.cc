#include "ChampSim/ramulator2_dram_controller.h"

#if (USER_CODES == ENABLE) && (RAMULATOR2 == ENABLE)

#include <cassert>
#include <cstdio>
#include <numeric>
#include <utility>

#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>
#endif

#include "ChampSim/champsim_constants.h"
#include "ChampSim/util/span.h"
#include "Ramulator2/base/base.h"
#include "Ramulator2/base/config.h"
#include "Ramulator2/base/factory.h"
#include "Ramulator2/frontend/frontend.h"
#include "Ramulator2/memory_system/memory_system.h"

MEMORY_CONTROLLER::MEMORY_CONTROLLER(champsim::chrono::picoseconds mc_period, std::vector<channel_type*>&& ul, std::string configs)
: champsim::operable(mc_period), queues(std::move(ul)), yaml_path(std::move(configs))
{
    const YAML::Node config = Ramulator::Config::parse_config_file(yaml_path, {});
    frontend                = Ramulator::Factory::create_frontend(config);
    memory_system           = Ramulator::Factory::create_memory_system(config);

    if (frontend == nullptr || memory_system == nullptr)
    {
        std::fprintf(stderr, "Ramulator 2.0: failed to build frontend / memory system from %s\n", yaml_path.c_str());
        std::abort();
    }

    const float memory_tCK = memory_system->get_tCK(); // Unit is nanosecond
    const float io_freq    = ONE_SECOND_IN_MILLISECOND / memory_tCK;
    std::printf("Memory IO frequency: %f MHz (Ramulator 2.0 backend).\n", io_freq);

    io_freq_scale = ((ONE_SECOND_IN_MICROSECOND / ONE_SECOND_IN_MILLISECOND) * memory_tCK) / mc_period.count();

    frontend->set_num_cores(NUM_CPUS);
    /** Set clock ratio */
    const uint32_t cpu_freq                = uint32_t(ONE_SECOND_IN_MICROSECOND / mc_period.count());
    const uint32_t memory_controller_freq  = uint32_t(io_freq);
    const uint32_t greatest_common_divisor = std::gcd(cpu_freq, memory_controller_freq);
    frontend->set_clock_ratio(cpu_freq / greatest_common_divisor);
    memory_system->set_clock_ratio(memory_controller_freq / greatest_common_divisor);

    frontend->connect_memory_system(memory_system);
    memory_system->connect_frontend(frontend);

    read_request_in_memory  = 0;
    write_request_in_memory = 0;
}

MEMORY_CONTROLLER::~MEMORY_CONTROLLER()
{
    // Print information to output_statistics
    output_statistics.read_request_in_memory  = read_request_in_memory;
    output_statistics.write_request_in_memory = write_request_in_memory;

    // Finalize the simulation. Recursively print all statistics from all components.
    // Ramulator 2.0 owns its components via internal lifetimes; we just print
    // stats before destruction. finalize() emits a YAML-formatted stats blob to
    // stdout (and into PRINT_STATISTICS_INTO_FILE downstream consumers via the
    // existing redirect, if enabled).
    if (frontend != nullptr)
    {
        frontend->finalize();
    }
    if (memory_system != nullptr)
    {
        memory_system->finalize();
    }
}

void MEMORY_CONTROLLER::initialize()
{
    using namespace champsim::data::data_literals;
    using namespace std::literals::chrono_literals;
    auto sz = this->size();

#if (USE_VCPKG == ENABLE)
    if (champsim::data::gibibytes gb_sz {sz}; gb_sz > 1_GiB)
    {
        fmt::print("Off-chip DRAM Size: {}", gb_sz);
    }
    else if (champsim::data::mebibytes mb_sz {sz}; mb_sz > 1_MiB)
    {
        fmt::print("Off-chip DRAM Size: {}", mb_sz);
    }
    else if (champsim::data::kibibytes kb_sz {sz}; kb_sz > 1_kiB)
    {
        fmt::print("Off-chip DRAM Size: {}", kb_sz);
    }
    else
    {
        fmt::print("Off-chip DRAM Size: {}", sz);
    }
    fmt::print(" Channels: {} Width: {}-bit Data Rate: {} MT/s\n", memory_system->get_channel(), memory_system->get_channel_width(), memory_system->get_rate());

#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    if (champsim::data::gibibytes gb_sz {sz}; gb_sz > 1_GiB)
    {
        std::fprintf(output_statistics.file_handler, "Off-chip DRAM Size: %lld", gb_sz.count());
    }
    else if (champsim::data::mebibytes mb_sz {sz}; mb_sz > 1_MiB)
    {
        std::fprintf(output_statistics.file_handler, "Off-chip DRAM Size: %lld", mb_sz.count());
    }
    else if (champsim::data::kibibytes kb_sz {sz}; kb_sz > 1_kiB)
    {
        std::fprintf(output_statistics.file_handler, "Off-chip DRAM Size: %lld", kb_sz.count());
    }
    else
    {
        std::fprintf(output_statistics.file_handler, "Off-chip DRAM Size: %lld", sz.count());
    }
    std::fprintf(output_statistics.file_handler, " Channels: %d Width: %d-bit Data Rate: %d MT/s\n",
        memory_system->get_channel(), memory_system->get_channel_width(), memory_system->get_rate());

#endif /* PRINT_STATISTICS_INTO_FILE */
}

long MEMORY_CONTROLLER::operate()
{
    long progress {0};

    initiate_requests();

    /* Operate memories below */
    static double leap_operation = 0;
    // Skip periodically
    if (leap_operation >= 1)
    {
        leap_operation -= 1;
    }
    else
    {
        memory_system->tick();
        leap_operation += io_freq_scale;

        ++progress;
    }

    return progress;
}

void MEMORY_CONTROLLER::begin_phase()
{
    for (auto* ul : queues)
    {
        channel_type::stats_type ul_new_roi_stats;
        channel_type::stats_type ul_new_sim_stats;
        ul->roi_stats = ul_new_roi_stats;
        ul->sim_stats = ul_new_sim_stats;
    }
}

void MEMORY_CONTROLLER::end_phase(unsigned)
{
    /* No code here */
}

void MEMORY_CONTROLLER::print_deadlock()
{
#if (USE_VCPKG == ENABLE)
    fmt::print("Memory controller (Ramulator 2.0) {}\n", __func__);
#endif /* USE_VCPKG */

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "Memory controller (Ramulator 2.0) %s\n", __func__);
#endif /* PRINT_STATISTICS_INTO_FILE */
}

champsim::data::bytes MEMORY_CONTROLLER::size() const
{
    return champsim::data::bytes {static_cast<long long>(memory_system->get_capacity())};
}

void MEMORY_CONTROLLER::initiate_requests()
{
    // Initiate read requests
    for (auto ul : queues)
    {
        for (auto q : {std::ref(ul->RQ), std::ref(ul->PQ)})
        {
            auto [begin, end] = champsim::get_span_p(std::cbegin(q.get()), std::cend(q.get()), [ul, this](const auto& pkt)
                { 
                    request_type packet = pkt;
                    return this->add_rq(packet, ul); }); // Add read requests
            q.get().erase(begin, end);
        }

        // Initiate write requests
        auto [wq_begin, wq_end] = champsim::get_span_p(std::cbegin(ul->WQ), std::cend(ul->WQ), [this](const auto& pkt)
            { 
                request_type packet = pkt;
                return this->add_wq(packet); }); // Add write requests
        ul->WQ.erase(wq_begin, wq_end);
    }
}

bool MEMORY_CONTROLLER::add_rq(const request_type& packet, champsim::channel* ul)
{
    const static int type            = Ramulator::Request::Type::Read; // It means the input request is read request.

    DRAM_CHANNEL::request_type rq_it = DRAM_CHANNEL::request_type {packet};
    rq_it.forward_checked            = false;
    rq_it.scheduled                  = false;
    rq_it.ready_time                 = current_time;

    if (packet.response_requested)
        rq_it.to_return = {&ul->returned}; // Store the response queue to communicate with the LLC

    /* Send memory request below */
    bool stall                 = true;

    const uint64_t max_address = this->size().count();
    const uint64_t address     = packet.address.to<uint64_t>();

    // Assign the request to the right memory.
    if (address < max_address)
    {
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), rq_it, memory_id);
        stall = ! memory_system->send(request);

        if (stall == false)
        {
            read_request_in_memory++;
        }
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif /* PRINT_MEMORY_TRACE */

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }

    return true;
}

bool MEMORY_CONTROLLER::add_wq(const request_type& packet)
{
    const static int type            = Ramulator::Request::Type::Write; // It means the input request is write request.

    DRAM_CHANNEL::request_type wq_it = DRAM_CHANNEL::request_type {packet};
    wq_it.forward_checked            = false;
    wq_it.scheduled                  = false;
    wq_it.ready_time                 = current_time;

    /* Send memory request below */
    bool stall                       = true;

    const uint64_t max_address       = this->size().count();
    const uint64_t address           = packet.address.to<uint64_t>();

    // Assign the request to the right memory.
    if (address < max_address)
    {
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), wq_it, memory_id);
        stall = ! memory_system->send(request);

        if (stall == false)
        {
            write_request_in_memory++;
        }
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // Output memory trace.
    output_memorytrace.output_memory_trace_hexadecimal(address, 'W');
#endif /* PRINT_MEMORY_TRACE */

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }

    return true;
}

size_t MEMORY_CONTROLLER::get_occupancy(RequestType queue_type, uint64_t address)
{
    const uint64_t max_address = this->size().count();

    // Assign the request to the right memory.
    if (address < max_address)
    {
        const static int type = static_cast<int>(queue_type);
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type);

        return memory_system->get_queue_occupancy(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

size_t MEMORY_CONTROLLER::get_queue_size(RequestType queue_type, uint64_t address)
{
    const uint64_t max_address = this->size().count();

    // Assign the request to the right memory.
    if (address < max_address)
    {
        const static int type = static_cast<int>(queue_type);
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type);

        return memory_system->get_queue_size(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

void MEMORY_CONTROLLER::return_data(Ramulator::Request& request)
{
    response_type response {request.packet.address, request.packet.v_address, request.packet.data, request.packet.pf_metadata, request.packet.instr_depend_on_me};

    for (auto ret : request.packet.to_return)
    {
        ret->push_back(response); // Fill the response into the response queue
    }
};

#endif /* USER_CODES && RAMULATOR2 */

/* Header */

#include "ChampSim/ramulator2_dram_controller.h"

#if (USER_CODES == ENABLE) && (RAMULATOR2 == ENABLE)

#include <cassert>
#include <cstdio>
#include <iostream>
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

/* Macro */

/* Type */

/* Prototype */

/* Variable */

/* Function */

#if (MEMORY_USE_HYBRID == ENABLE)
// Enable hybrid memory system

MEMORY_CONTROLLER::MEMORY_CONTROLLER(champsim::chrono::picoseconds mc_period, std::vector<channel_type*>&& ul, std::string configs, std::string configs2)
: champsim::operable(mc_period), queues(std::move(ul)), yaml_path(std::move(configs)), yaml_path2(std::move(configs2))
{
    const uint32_t cpu_freq = uint32_t(ONE_SECOND_IN_MICROSECOND / mc_period.count());

    // Build the fast memory (memory_id == MEMORY_NUMBER_ONE)
    {
        const YAML::Node config = Ramulator::Config::parse_config_file(yaml_path, {});
        frontend                = Ramulator::Factory::create_frontend(config);
        memory_system           = Ramulator::Factory::create_memory_system(config);

        if (frontend == nullptr || memory_system == nullptr)
        {
            std::fprintf(stderr, "Ramulator 2.0: failed to build fast frontend / memory system from %s\n", yaml_path.c_str());
            std::abort();
        }

        const float memory_tCK = memory_system->get_tCK(); // Unit is nanosecond
        const float io_freq    = ONE_SECOND_IN_MILLISECOND / memory_tCK;
        std::printf("Memory IO frequency: %f MHz (Ramulator 2.0 backend, fast memory).\n", io_freq);

        io_freq_scale = ((ONE_SECOND_IN_MICROSECOND / ONE_SECOND_IN_MILLISECOND) * memory_tCK) / mc_period.count();

        frontend->set_num_cores(NUM_CPUS);
        const uint32_t memory_controller_freq  = uint32_t(io_freq);
        const uint32_t greatest_common_divisor = std::gcd(cpu_freq, memory_controller_freq);
        frontend->set_clock_ratio(cpu_freq / greatest_common_divisor);
        memory_system->set_clock_ratio(memory_controller_freq / greatest_common_divisor);

        frontend->connect_memory_system(memory_system);
        memory_system->connect_frontend(frontend);

        max_address = memory_system->get_capacity();
    }

    // Build the slow memory (memory_id == MEMORY_NUMBER_TWO)
    {
        const YAML::Node config = Ramulator::Config::parse_config_file(yaml_path2, {});
        frontend2               = Ramulator::Factory::create_frontend(config);
        memory_system2          = Ramulator::Factory::create_memory_system(config);

        if (frontend2 == nullptr || memory_system2 == nullptr)
        {
            std::fprintf(stderr, "Ramulator 2.0: failed to build slow frontend / memory system from %s\n", yaml_path2.c_str());
            std::abort();
        }

        const float memory_tCK = memory_system2->get_tCK(); // Unit is nanosecond
        const float io_freq    = ONE_SECOND_IN_MILLISECOND / memory_tCK;
        std::printf("Memory IO frequency 2: %f MHz (Ramulator 2.0 backend, slow memory).\n", io_freq);

        io_freq_scale2 = ((ONE_SECOND_IN_MICROSECOND / ONE_SECOND_IN_MILLISECOND) * memory_tCK) / mc_period.count();

        frontend2->set_num_cores(NUM_CPUS);
        const uint32_t memory_controller_freq  = uint32_t(io_freq);
        const uint32_t greatest_common_divisor = std::gcd(cpu_freq, memory_controller_freq);
        frontend2->set_clock_ratio(cpu_freq / greatest_common_divisor);
        memory_system2->set_clock_ratio(memory_controller_freq / greatest_common_divisor);

        frontend2->connect_memory_system(memory_system2);
        memory_system2->connect_frontend(frontend2);

        max_address2 = memory_system2->get_capacity();
    }

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management = new OS_TRANSPARENT_MANAGEMENT(max_address + max_address2, max_address);
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    swapping_count = swapping_traffic_in_bytes = 0;
#endif /* MEMORY_USE_SWAPPING_UNIT */

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    load_request_in_memory = load_request_in_memory2 = 0;
    store_request_in_memory = store_request_in_memory2 = 0;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    read_request_in_memory = read_request_in_memory2 = 0;
    write_request_in_memory = write_request_in_memory2 = 0;

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    initialize_swapping();

#if (TEST_SWAPPING_UNIT == ENABLE)
    for (auto i = 0; i < MEMORY_DATA_NUMBER; i++)
    {
        memory_data[i][0] = i;
    }
#endif /* TEST_SWAPPING_UNIT */
#endif /* MEMORY_USE_SWAPPING_UNIT */
}

MEMORY_CONTROLLER::~MEMORY_CONTROLLER()
{
    // Print information to output_statistics
#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    output_statistics.swapping_count            = swapping_count;
    output_statistics.swapping_traffic_in_bytes = swapping_traffic_in_bytes;
#endif /* MEMORY_USE_SWAPPING_UNIT */

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    output_statistics.load_request_in_memory   = load_request_in_memory;
    output_statistics.store_request_in_memory  = store_request_in_memory;
    output_statistics.load_request_in_memory2  = load_request_in_memory2;
    output_statistics.store_request_in_memory2 = store_request_in_memory2;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    output_statistics.read_request_in_memory   = read_request_in_memory;
    output_statistics.read_request_in_memory2  = read_request_in_memory2;
    output_statistics.write_request_in_memory  = write_request_in_memory;
    output_statistics.write_request_in_memory2 = write_request_in_memory2;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    delete os_transparent_management;
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    // Finalize the simulation. Recursively print all statistics from all components.
    if (frontend != nullptr)
    {
        frontend->finalize();
    }
    if (memory_system != nullptr)
    {
        memory_system->finalize();
    }
    if (frontend2 != nullptr)
    {
        frontend2->finalize();
    }
    if (memory_system2 != nullptr)
    {
        memory_system2->finalize();
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
    fmt::print(" Channels: {}, {} Width: {}-bit, {}-bit Data Rate: {} MT/s, {} MT/s\n",
        memory_system->get_channel(), memory_system2->get_channel(), memory_system->get_channel_width(), memory_system2->get_channel_width(), memory_system->get_rate(), memory_system2->get_rate());

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
    std::fprintf(output_statistics.file_handler, " Channels: %d, %d Width: %d-bit, %d-bit Data Rate: %d MT/s, %d MT/s\n",
        memory_system->get_channel(), memory_system2->get_channel(), memory_system->get_channel_width(), memory_system2->get_channel_width(), memory_system->get_rate(), memory_system2->get_rate());

#endif /* PRINT_STATISTICS_INTO_FILE */
}

long MEMORY_CONTROLLER::operate()
{
    long progress {0};

    initiate_requests();

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->cold_data_detection();

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    for (size_t i = 0; i < os_transparent_management->incomplete_read_request_queue.size(); i++)
    {
        if (os_transparent_management->incomplete_read_request_queue[i].fm_access_finish) // This read request is ready to access slow memory
        {
            request_type packet              = os_transparent_management->incomplete_read_request_queue[i].packet;

            DRAM_CHANNEL::request_type rq_it = DRAM_CHANNEL::request_type {packet};
            rq_it.forward_checked            = false;
            rq_it.event_cycle                = current_cycle;
            if (packet.response_requested)
                rq_it.to_return = {&(queues.at(packet.cpu)->returned)}; // Store the response queue to communicate with the LLC

            /* Send memory request below */
            bool stall       = true;

            uint64_t address = packet.h_address;
            if ((max_address <= address) && (address < max_address + max_address2))
            {
                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), Ramulator::Request::Type::Read, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), rq_it, memory2_id);
                stall = ! memory_system2->send(request);

                if (stall == false)
                {
                    read_request_in_memory2++;
                    os_transparent_management->incomplete_read_request_queue.erase(os_transparent_management->incomplete_read_request_queue.begin() + i);
                }
            }
            else
            {
                std::printf("%s: Error!\n", __FUNCTION__);
            }
        }
    }

    for (size_t i = 0; i < os_transparent_management->incomplete_write_request_queue.size(); i++)
    {
        if (os_transparent_management->incomplete_write_request_queue[i].fm_access_finish) // This write request is ready to write memory (fast or slow)
        {
            request_type packet              = os_transparent_management->incomplete_write_request_queue[i].packet;

            DRAM_CHANNEL::request_type wq_it = DRAM_CHANNEL::request_type {packet};
            wq_it.forward_checked            = false;
            wq_it.event_cycle                = current_cycle;

            /* Send memory request below */
            bool stall                       = true;

            uint64_t address                 = packet.h_address;

            // Assign the request to the right memory.
            if (address < max_address)
            {
                Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), Ramulator::Request::Type::Write, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), wq_it, memory_id);
                stall = ! memory_system->send(request);

                if (stall == false)
                {
                    write_request_in_memory++;
                    os_transparent_management->incomplete_write_request_queue.erase(os_transparent_management->incomplete_write_request_queue.begin() + i);
                }
            }
            else if (address < max_address + max_address2)
            {
                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), Ramulator::Request::Type::Write, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), wq_it, memory2_id);
                stall = ! memory_system2->send(request);

                if (stall == false)
                {
                    write_request_in_memory2++;
                    os_transparent_management->incomplete_write_request_queue.erase(os_transparent_management->incomplete_write_request_queue.begin() + i);
                }
            }
            else
            {
                std::printf("%s: Error!\n", __FUNCTION__);
            }
        }
    }
#endif /* COLOCATED_LINE_LOCATION_TABLE */
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Operate swapping below */
    uint8_t swapping_states = operate_swapping();
    switch (swapping_states)
    {
    case 0: // The swapping unit is idle
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management->issue_remapping_request(remapping_request);
        if (issue == true) // Get a new remapping request.
        {
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            start_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            start_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif /* IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD */
        }
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
    }
    break;
    case 1: // The swapping unit is busy
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management->issue_remapping_request(remapping_request);
        if (issue == true) // Get a remapping request.
        {
            // In case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif /* IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD */
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error." << std::endl;
            assert(false);
        }
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
    }
    break;
    case 2: // The swapping unit finishes a swapping request
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        bool is_updated = false;
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management->issue_remapping_request(remapping_request);
        if (issue == true) // Get a remapping request.
        {
            // In case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            is_updated = update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            is_updated = update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif /* IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD */
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error 2." << std::endl;
            assert(false);
        }

        if (is_updated == false)
        {
            os_transparent_management->finish_remapping_request();
            initialize_swapping();
        }
#else

        initialize_swapping();
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
    }
    break;
    default:
        break;
    }
#else

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    // Since we don't consider data swapping overhead here, the data swapping is finished immediately
    OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
    bool issue = os_transparent_management->issue_remapping_request(remapping_request);
    if (issue == true) // Get a new remapping request.
    {
        os_transparent_management->finish_remapping_request();
    }
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */
#endif /* MEMORY_USE_SWAPPING_UNIT */

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#if (IDEAL_SINGLE_MEMPOD == ENABLE)
    os_transparent_management->check_interval_swap(swapping_states, warmup);
#endif /* IDEAL_SINGLE_MEMPOD */
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    /* Operate memories below */
    static double leap_operation = 0, leap_operation2 = 0;
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

    if (leap_operation2 >= 1)
    {
        leap_operation2 -= 1;
    }
    else
    {
        memory_system2->tick();
        leap_operation2 += io_freq_scale2;

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

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    std::printf("base_address[0]: %ld, base_address[1]: %ld.\n", base_address[0], base_address[1]);
#endif /* MEMORY_USE_SWAPPING_UNIT */
}

champsim::data::bytes MEMORY_CONTROLLER::size() const
{
    return champsim::data::bytes {static_cast<long long>(max_address + max_address2)};
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

bool MEMORY_CONTROLLER::add_rq(request_type& packet, champsim::channel* ul)
{
    const static int type                                              = Ramulator::Request::Type::Read;                     // It means the input request is read request (Ramulator 2.0 id).
    const static OS_TRANSPARENT_MANAGEMENT::MemoryRequestType ost_type = OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read; // The matching id for OS-transparent management.
    const static RequestType queue_type                                = RequestType::Read;                                  // The matching id for queue queries.

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    access_type type_origin = packet.type_origin;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->physical_to_hardware_address(packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management->memory_activity_tracking(packet.address.to<uint64_t>(), ost_type, packet.type_origin, float(get_occupancy(queue_type, packet.address.to<uint64_t>())) / get_queue_size(queue_type, packet.address.to<uint64_t>()));
#else
    os_transparent_management->memory_activity_tracking(packet.address.to<uint64_t>(), ost_type, float(get_occupancy(queue_type, packet.address.to<uint64_t>())) / get_queue_size(queue_type, packet.address.to<uint64_t>()));
#endif /* TRACKING_LOAD_STORE_STATISTICS */
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(packet, ost_type);
    switch (under_swapping)
    {
    case 0: // This address is under swapping.
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    break;
    case 1: // This address is not under swapping.
        break;
    case 2: // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
    {
        response_type response {packet.address, packet.v_address, packet.data, packet.pf_metadata, packet.instr_depend_on_me};

        for (auto ret : {&ul->returned})
        {
            ret->push_back(response); // Fill the response into the response queue
        }

        return true; // Fast-forward
    }
    break;
    default:
        break;
    }
#endif /* MEMORY_USE_SWAPPING_UNIT */

    DRAM_CHANNEL::request_type rq_it = DRAM_CHANNEL::request_type {packet};
    rq_it.forward_checked            = false;
    rq_it.scheduled                  = false;
    rq_it.ready_time                 = current_time;

    if (packet.response_requested)
        rq_it.to_return = {&ul->returned}; // Store the response queue to communicate with the LLC

    /* Send memory request below */
    bool stall = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = rq_it.h_address = packet.h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool is_sm_request = false; // Whether this request is mapped in slow memory according to the LLT.
    if (max_address <= address)
    {
        is_sm_request = true;
        address = rq_it.h_address_fm = packet.h_address_fm; // Pretend to access the Location Entry and Data (LEAD) in fast memory

        if (max_address <= address)
        {
            std::cout << __func__ << ": co_located LLT error, h_address_fm is uncorrect." << std::endl;
            abort();
        }

        // Check incomplete_read_request_queue's size
        if (os_transparent_management->incomplete_read_request_queue.size() >= INCOMPLETE_READ_REQUEST_QUEUE_LENGTH)
        {
            return false;
        }
    }
#endif /* COLOCATED_LINE_LOCATION_TABLE */

#else
    address = packet.address.to<uint64_t>();
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    // Assign the request to the right memory.
    if (address < max_address)
    {
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), rq_it, memory_id);
        stall = ! memory_system->send(request);

        if (stall == false)
        {
            read_request_in_memory++;

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
            if (warmup == false)
            {
                if (type_origin == access_type::LOAD || type_origin == access_type::TRANSLATION)
                {
                    load_request_in_memory++;
                }
                else if (type_origin == access_type::RFO)
                {
                    store_request_in_memory++;
                }
            }
#endif /* TRACKING_LOAD_STORE_STATISTICS */

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
            if (is_sm_request)
            {
                read_request_in_memory--;

                OS_TRANSPARENT_MANAGEMENT::ReadRequest read_request;
                // Create new read_request
                read_request.packet           = packet;
                read_request.fm_access_finish = false;

                os_transparent_management->incomplete_read_request_queue.push_back(read_request);
            }
#endif /* COLOCATED_LINE_LOCATION_TABLE */
        }
    }
    else if (address < max_address + max_address2)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), type, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), rq_it, memory2_id);
        stall = ! memory_system2->send(request);

        if (stall == false)
        {
            read_request_in_memory2++;
        }

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
        if (type_origin == access_type::LOAD || type_origin == access_type::TRANSLATION)
        {
            load_request_in_memory2++;
        }
        else if (type_origin == access_type::RFO)
        {
            store_request_in_memory2++;
        }
        else
        {
            std::printf("%s: Error!\n", __FUNCTION__);
        }
#endif /* TRACKING_LOAD_STORE_STATISTICS */
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // Output memory trace
    output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif /* PRINT_MEMORY_TRACE */

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }

    return true;
}

bool MEMORY_CONTROLLER::add_wq(request_type& packet)
{
    const static int type                                              = Ramulator::Request::Type::Write;                     // It means the input request is write request (Ramulator 2.0 id).
    const static OS_TRANSPARENT_MANAGEMENT::MemoryRequestType ost_type = OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Write; // The matching id for OS-transparent management.
    const static RequestType queue_type                                = RequestType::Write;                                  // The matching id for queue queries.

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->physical_to_hardware_address(packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management->memory_activity_tracking(packet.address.to<uint64_t>(), ost_type, packet.type_origin, float(get_occupancy(queue_type, packet.address.to<uint64_t>())) / get_queue_size(queue_type, packet.address.to<uint64_t>()));
#else
    os_transparent_management->memory_activity_tracking(packet.address.to<uint64_t>(), ost_type, float(get_occupancy(queue_type, packet.address.to<uint64_t>())) / get_queue_size(queue_type, packet.address.to<uint64_t>()));
#endif /* TRACKING_LOAD_STORE_STATISTICS */
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(packet, ost_type);
    switch (under_swapping)
    {
    case 0: // This address is under swapping.
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    break;
    case 1: // This address is not under swapping.
        break;
    case 2: // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
    {
        return true; // Fast-forward
    }
    break;
    default:
        break;
    }
#endif /* MEMORY_USE_SWAPPING_UNIT */

    DRAM_CHANNEL::request_type wq_it = DRAM_CHANNEL::request_type {packet};
    wq_it.forward_checked            = false;
    wq_it.scheduled                  = false;
    wq_it.ready_time                 = current_time;

    /* Send memory request below */
    bool stall                       = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = wq_it.h_address = packet.h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    // Check incomplete_write_request_queue's size
    if (os_transparent_management->incomplete_write_request_queue.size() >= INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH)
    {
        return false;
    }

    address = wq_it.h_address_fm = packet.h_address_fm; // Pretend to access the Location Entry and Data (LEAD) in fast memory
    Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), Ramulator::Request::Type::Read, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), wq_it, memory_id);
    stall = ! memory_system->send(request);

    if (stall == false)
    {
        OS_TRANSPARENT_MANAGEMENT::WriteRequest write_request;
        // Create new write_request
        write_request.packet           = packet;
        write_request.fm_access_finish = false;

        os_transparent_management->incomplete_write_request_queue.push_back(write_request);

        return true;
    }
    else
    {
        return false;
    }
#endif /* COLOCATED_LINE_LOCATION_TABLE */

#else
    address = packet.address.to<uint64_t>();
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

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
    else if (address < max_address + max_address2)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), type, static_cast<int>(packet.cpu), std::bind(&MEMORY_CONTROLLER::return_data, this, std::placeholders::_1), wq_it, memory2_id);
        stall = ! memory_system2->send(request);

        if (stall == false)
        {
            write_request_in_memory2++;
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
    const int type = static_cast<int>(queue_type);
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->physical_to_hardware_address(address);
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    // Assign the request to the right memory.
    if (address < max_address)
    {
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type);
        return memory_system->get_queue_occupancy(request);
    }
    else if (address < max_address + max_address2)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), type);
        return memory_system2->get_queue_occupancy(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

size_t MEMORY_CONTROLLER::get_queue_size(RequestType queue_type, uint64_t address)
{
    const int type = static_cast<int>(queue_type);
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->physical_to_hardware_address(address);
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    // Assign the request to the right memory.
    if (address < max_address)
    {
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), type);
        return memory_system->get_queue_size(request);
    }
    else if (address < max_address + max_address2)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), type);
        return memory_system2->get_queue_size(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

void MEMORY_CONTROLLER::return_data(Ramulator::Request& request)
{
    // Recover the hardware address to physical address.
    switch (request.memory_id)
    {
    case MEMORY_NUMBER_ONE:
        // Nothing to do
        break;
    case MEMORY_NUMBER_TWO:
        request.addr += max_address;
        break;
    default:
    {
        std::cout << __func__ << ": return_data error." << std::endl;
        assert(false);
    }
    break;
    }

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool finish_return_data = false;

    if (uint64_t(request.addr) < max_address)
    {
        // This could be an uncomplete write request
        bool finish = os_transparent_management->finish_fm_access_in_incomplete_write_request_queue(request.packet.h_address);
        if (finish)
        {
            finish_return_data = true;
            return;
        }
    }

    if ((uint64_t(request.addr) < max_address) && (max_address <= request.packet.h_address))
    {
        // This could be an uncomplete read request
        bool finish = os_transparent_management->finish_fm_access_in_incomplete_read_request_queue(request.packet.h_address);

        if (finish)
        {
            finish_return_data = true;
            return;
        }
    }

    if (finish_return_data == false)
    {
        // This is a complete read request
        response_type response {request.packet.address, request.packet.v_address, request.packet.data, request.packet.pf_metadata, request.packet.instr_depend_on_me};

        for (auto ret : request.packet.to_return)
        {
            ret->push_back(response); // Fill the response into the response queue
        }
    }

#else
    response_type response {request.packet.address, request.packet.v_address, request.packet.data, request.packet.pf_metadata, request.packet.instr_depend_on_me};

    for (auto ret : request.packet.to_return)
    {
        ret->push_back(response); // Fill the response into the response queue
    }
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT && COLOCATED_LINE_LOCATION_TABLE */
};

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)

// Functions for swapping:
void MEMORY_CONTROLLER::initialize_swapping()
{
    swapping_count += active_entry_number * 2;
    swapping_traffic_in_bytes += active_entry_number * 2 * BLOCK_SIZE;

    states = SwappingState::Idle;
    for (auto i = 0; i < SWAPPING_BUFFER_ENTRY_NUMBER; i++)
    {
        buffer[i].finish = false;
        for (auto j = 0; j < SWAPPING_SEGMENT_NUMBER; j++)
        {
            for (uint64_t z = 0; z < BLOCK_SIZE; z++)
            {
                buffer[i].data[j][z] = 0;
            }
            buffer[i].read_issue[j] = false;
            buffer[i].read[j]       = false;
            buffer[i].write[j]      = false;
            buffer[i].dirty[j]      = false;
        }
    }

    for (auto i = 0; i < SWAPPING_SEGMENT_NUMBER; i++)
    {
        base_address[i] = 0;
    }
    active_entry_number = finish_number = 0;
}

// Input address should be hardware address and at byte granularity
bool MEMORY_CONTROLLER::start_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
{
    assert(size <= SWAPPING_BUFFER_ENTRY_NUMBER);

    if (states == SwappingState::Idle)
    {
        states              = SwappingState::Swapping;      // Start swapping.
        base_address[0]     = address_1 >> LOG2_BLOCK_SIZE; // The single swapping is conducted at cache line granularity.
        base_address[1]     = address_2 >> LOG2_BLOCK_SIZE;
        active_entry_number = size;
    }
    else
    {
        return false; // This swapping unit is busy, it cannot issue new swapping request.
    }
    return true; // New swapping is issued.
}

// Input address should be hardware address and at byte granularity
bool MEMORY_CONTROLLER::update_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
{
    assert(size <= SWAPPING_BUFFER_ENTRY_NUMBER);

    uint64_t input_base_address[SWAPPING_SEGMENT_NUMBER];

    input_base_address[0] = address_1 >> LOG2_BLOCK_SIZE;
    input_base_address[1] = address_2 >> LOG2_BLOCK_SIZE;

    if ((input_base_address[0] == base_address[0]) && (input_base_address[1] == base_address[1]))
    {
        // They are same swapping segments
        if (size > active_entry_number)
        {
            // There have new data to swap
            active_entry_number = size;
            if (states == SwappingState::Idle)
            {
                states = SwappingState::Swapping; // Start swapping.
            }

            return true; // Update swapping segments
        }
    }

    return false;
}

uint8_t MEMORY_CONTROLLER::operate_swapping()
{
    const static int coreid = 0;

    switch (states)
    {
    case SwappingState::Idle:
    {
        return 0; // Idle
    }
    break;
    case SwappingState::Swapping:
    {
        // Issue read requests
        for (auto i = 0; i < active_entry_number; i++) // Go through the active buffer
        {
            if (buffer[i].finish == false)
            {
                for (auto j = 0; j < SWAPPING_SEGMENT_NUMBER; j++)
                {
                    if (buffer[i].read_issue[j] == false)
                    {
                        bool stall       = true;
                        uint64_t address = (base_address[j] + i) << LOG2_BLOCK_SIZE;

                        // Assign the request to the right memory.
                        if (address < max_address)
                        {
                            Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), Ramulator::Request::Type::Read, coreid, std::bind(&MEMORY_CONTROLLER::return_swapping_data, this, std::placeholders::_1), memory_id);
                            stall = ! memory_system->send(request);
                        }
                        else if (address < max_address + max_address2)
                        {
                            // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                            Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), Ramulator::Request::Type::Read, coreid, std::bind(&MEMORY_CONTROLLER::return_swapping_data, this, std::placeholders::_1), memory2_id);
                            stall = ! memory_system2->send(request);
                        }
                        else
                        {
                            std::printf("%s: Error!\n", __FUNCTION__);
                        }

#if (PRINT_MEMORY_TRACE == ENABLE)
                        // Output memory trace.
                        output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif /* PRINT_MEMORY_TRACE */

                        if (stall == true)
                        {
                            // Queue is full
                        }
                        else
                        {
                            buffer[i].read_issue[j] = true;
                        }
                    }
                }
            }
        }

        // Issue write requests
        for (auto i = 0; i < active_entry_number; i++) // Go through the active buffer
        {
            if (buffer[i].finish == false)
            {
                if ((buffer[i].read[0] == true) && (buffer[i].read[1] == true)) // Read requests are finished
                {
                    for (auto j = 0; j < SWAPPING_SEGMENT_NUMBER; j++)
                    {
                        if ((buffer[i].write[j] == false) || (buffer[i].dirty[j] == true))
                        {
                            bool stall       = true;
                            uint64_t address = (base_address[j] + i) << LOG2_BLOCK_SIZE;

                            // Assign the request to the right memory.
                            if (address < max_address)
                            {
                                Ramulator::Request request(static_cast<Ramulator::Addr_t>(address), Ramulator::Request::Type::Write, coreid, nullptr, memory_id);
                                // Get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory_system->send(request);
                            }
                            else if (address < max_address + max_address2)
                            {
                                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                                Ramulator::Request request(static_cast<Ramulator::Addr_t>(address - max_address), Ramulator::Request::Type::Write, coreid, nullptr, memory2_id);
                                // Get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory_system2->send(request);
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
                                // Queue is full
                            }
                            else
                            {
                                if (buffer[i].write[j] == false)
                                {
                                    buffer[i].write[j] = true;
                                }
                                if (buffer[i].dirty[j] == true)
                                {
                                    buffer[i].dirty[j] = false;
                                }

#if (TEST_SWAPPING_UNIT == ENABLE)
                                memory_data[i + j * MEMORY_DATA_NUMBER / 2] = buffer[i].data[j];
#endif /* TEST_SWAPPING_UNIT */
                            }
                        }
                    }
                }

                // Finish swapping
                if ((buffer[i].write[0] == true) && (buffer[i].write[1] == true))
                {
                    buffer[i].finish = true;
                    finish_number++;
                }
            }
        }

        // Check finish_number
        if (finish_number == active_entry_number)
        {
            states = SwappingState::Idle;
            return 2; // Finished swapping
        }

        return 1; // Swapping
    }
    break;
    default:
        break;
    }

    return 3; // No meaning, it should never come here.
}

// This function is used by memories, like Ramulator.
void MEMORY_CONTROLLER::return_swapping_data(Ramulator::Request& request)
{
    // Sanity check
    // assert(states == SwappingState::Swapping);
    if (states == SwappingState::Idle)
    {
        std::cout << __func__ << ": data is returned while swapping is already finished." << std::endl;
        return;
    }

    // Recover the hardware address to physical address.
    switch (request.memory_id)
    {
    case MEMORY_NUMBER_ONE:
        // Nothing to do
        break;
    case MEMORY_NUMBER_TWO:
        request.addr += max_address;
        break;
    default:
    {
        std::cout << __func__ << ": swapping error." << std::endl;
        assert(false);
    }
    break;
    }

    uint64_t address = request.addr >> LOG2_BLOCK_SIZE;
    uint8_t segment_index;
    uint8_t entry_index;
    // Calculate entry index in the fashion of little-endian.
    if ((base_address[SWAPPING_SEGMENT_ONE] <= address) && (address < (base_address[SWAPPING_SEGMENT_ONE] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_TWO;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_ONE]);

#if (TEST_SWAPPING_UNIT == ENABLE)
        request.data = memory_data[entry_index];
#endif /* TEST_SWAPPING_UNIT */
    }
    else if ((base_address[SWAPPING_SEGMENT_TWO] <= address) && (address < (base_address[SWAPPING_SEGMENT_TWO] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_ONE;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_TWO]);

#if (TEST_SWAPPING_UNIT == ENABLE)
        request.data = memory_data[entry_index + MEMORY_DATA_NUMBER / 2];
#endif /* TEST_SWAPPING_UNIT */
    }
    else
    {
        std::cout << __func__ << ": swapping error." << std::endl;
        assert(false);
    }

    // Read data
    if ((buffer[entry_index].finish == false) && (buffer[entry_index].write[segment_index] == false) && (buffer[entry_index].dirty[segment_index] == false))
    {
        buffer[entry_index].data[segment_index] = request.data;
        buffer[entry_index].read[segment_index] = true;
    }
};

uint8_t MEMORY_CONTROLLER::check_request(request_type& packet, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType type)
{
    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = packet.h_address;
#else
    address = packet.address;
#endif

    address >>= LOG2_BLOCK_SIZE;
    uint8_t segment_index;
    uint8_t entry_index;
    // Calculate entry index in the fashion of little-endian.
    if ((base_address[SWAPPING_SEGMENT_ONE] <= address) && (address < (base_address[SWAPPING_SEGMENT_ONE] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_ONE;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_ONE]);
    }
    else if ((base_address[SWAPPING_SEGMENT_TWO] <= address) && (address < (base_address[SWAPPING_SEGMENT_TWO] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_TWO;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_TWO]);
    }
    else
    {
        return 1; // This address is not under swapping.
    }

    if (type == OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Read) // For read request
    {
        if ((buffer[entry_index].finish == true) || (buffer[entry_index].read[segment_index] == true) || (buffer[entry_index].write[segment_index] == true) || (buffer[entry_index].dirty[segment_index] == true))
        {
            uint64_t& read_data = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // Note the PACKET only has 64 bit data.
            packet.data         = champsim::address {read_data};

            return 2; // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else if (type == OS_TRANSPARENT_MANAGEMENT::MemoryRequestType::Write) // For write request
    {
        if ((buffer[entry_index].read[0] == true) && (buffer[entry_index].read[1] == true))
        {
            uint64_t& write_data                     = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // Note the PACKET only has 64 bit data.
            write_data                               = packet.data.to<uint64_t>();
            buffer[entry_index].dirty[segment_index] = true;

            if (buffer[entry_index].finish == true)
            {
                --finish_number;
            }
            buffer[entry_index].finish = false;

            return 2; // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else
    {
        std::cout << __func__ << ": type input error." << std::endl;
        assert(false);
    }

    return 0; // This address is under swapping.
};

uint8_t MEMORY_CONTROLLER::check_address(uint64_t address, uint8_t type)
{
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management->physical_to_hardware_address(address);
#else
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

    address >>= LOG2_BLOCK_SIZE;
    uint8_t segment_index;
    uint8_t entry_index;
    // Calculate entry index in the fashion of little-endian.
    if ((base_address[SWAPPING_SEGMENT_ONE] <= address) && (address < (base_address[SWAPPING_SEGMENT_ONE] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_ONE;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_ONE]);
    }
    else if ((base_address[SWAPPING_SEGMENT_TWO] <= address) && (address < (base_address[SWAPPING_SEGMENT_TWO] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_TWO;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_TWO]);
    }
    else
    {
        return 1; // This address is not under swapping.
    }

    if (type == 1) // For read request
    {
        if ((buffer[entry_index].finish == true) || (buffer[entry_index].read[segment_index] == true) || (buffer[entry_index].write[segment_index] == true) || (buffer[entry_index].dirty[segment_index] == true))
        {
            return 2; // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else if (type == 2) // For write request
    {
        if ((buffer[entry_index].read[0] == true) && (buffer[entry_index].read[1] == true))
        {
            return 2; // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else
    {
        std::cout << __func__ << ": type input error." << std::endl;
        assert(false);
    }

    return 0; // This address is under swapping.
};

#endif /* MEMORY_USE_SWAPPING_UNIT */

#else
// Disable hybrid memory system

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

    max_address             = memory_system->get_capacity();

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
    return champsim::data::bytes {static_cast<long long>(max_address)};
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
    bool stall             = true;

    const uint64_t address = packet.address.to<uint64_t>();

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

#endif /* MEMORY_USE_HYBRID */

#endif /* USER_CODES && RAMULATOR2 */

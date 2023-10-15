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

#ifndef DRAM_CONTROLLER_H
#define DRAM_CONTROLLER_H

/* Header */
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include "ChampSim/champsim_constants.h"
#include "ChampSim/channel.h"
#include "ChampSim/operable.h"
#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

#if (USE_VCPKG == ENABLE)
#include <fmt/core.h>
#endif // USE_VCPKG

#include <cassert>
#include <cstdint>
#include <iostream>

#include "ChampSim/util/span.h"
#include "os_transparent_management.h"

// Define the MEMORY_CONTROLLER class
#if (RAMULATOR == ENABLE)
#include "Ramulator/Memory.h"
#include "Ramulator/Request.h"

/* Macro */

/* Type */

/* Prototype */
#if (MEMORY_USE_HYBRID == ENABLE)
template<typename T, typename T2>
class MEMORY_CONTROLLER : public champsim::operable
{
    double clock_scale  = MEMORY_CONTROLLER_CLOCK_SCALE;
    double clock_scale2 = MEMORY_CONTROLLER_CLOCK_SCALE;

    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    void initiate_requests();
    bool add_rq(request_type& pkt, champsim::channel* ul);
    bool add_wq(request_type& pkt);

public:
    // Note here they are the references to escape memory deallocation here.
    ramulator::Memory<T, ramulator::Controller>& memory;
    ramulator::Memory<T2, ramulator::Controller>& memory2;

    const uint8_t memory_id  = MEMORY_NUMBER_ONE;
    const uint8_t memory2_id = MEMORY_NUMBER_TWO;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    OS_TRANSPARENT_MANAGEMENT& os_transparent_management;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Swapping unit */
    struct BUFFER_ENTRY
    {
        bool finish                                                   = false; // Whether a swapping of this entry is completed.

        std::array<uint8_t, BLOCK_SIZE> data[SWAPPING_SEGMENT_NUMBER] = {0}; // Cache lines
        bool read_issue[SWAPPING_SEGMENT_NUMBER];                            // Whether a read request is issued
        bool read[SWAPPING_SEGMENT_NUMBER];                                  // Whether a read request is finished
        bool write[SWAPPING_SEGMENT_NUMBER];                                 // Whether a write request is finished
        bool dirty[SWAPPING_SEGMENT_NUMBER];                                 // Whether a "new" write request is received
    };

    std::array<BUFFER_ENTRY, SWAPPING_BUFFER_ENTRY_NUMBER> buffer = {};
    uint64_t base_address[SWAPPING_SEGMENT_NUMBER]; // Here base_address[0] for segment 1, base_address[1] for segment 2. Address is hardware address and at cache line granularity.
    uint8_t active_entry_number;
    uint8_t finish_number;

    // Scoped enumerations
    enum class SwappingState : uint8_t {
        Idle,
        Swapping};
    SwappingState states; // The state of swapping mechanism

    uint64_t swapping_count;
    uint64_t swapping_traffic_in_bytes;

#if (TEST_SWAPPING_UNIT == ENABLE)
#define MEMORY_DATA_NUMBER (128)
    std::array<uint8_t, BLOCK_SIZE> memory_data[MEMORY_DATA_NUMBER] = {0};
#endif // TEST_SWAPPING_UNIT

#endif // MEMORY_USE_SWAPPING_UNIT

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    uint64_t load_request_in_memory, load_request_in_memory2;
    uint64_t store_request_in_memory, store_request_in_memory2;
#endif // TRACKING_LOAD_STORE_STATISTICS

    uint64_t read_request_in_memory, read_request_in_memory2;
    uint64_t write_request_in_memory, write_request_in_memory2;

    /* Member functions */
    MEMORY_CONTROLLER(double freq_scale, double clock_scale, double clock_scale2, std::vector<channel_type*>&& ul, ramulator::Memory<T, ramulator::Controller>& memory, ramulator::Memory<T2, ramulator::Controller>& memory2);
    ~MEMORY_CONTROLLER();

    void initialize() override final;
    long operate() override final;
    void begin_phase() override final;
    void end_phase(unsigned cpu) override final;
    void print_deadlock() override final;

    std::size_t size() const;

    /** @brief
     *  Get the number of valid member in the related write/read queue.
     *  The address is physical address.
     */
    uint32_t get_occupancy(ramulator::Request::Type queue_type, uint64_t address);

    /** @brief
     *  Get the capacity of the related write/read queue.
     *  The address is physical address.
     */
    uint32_t get_size(ramulator::Request::Type queue_type, uint64_t address);

    void return_data(ramulator::Request& request);

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
public:
    // Input address should be hardware address and at byte granularity
    bool start_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size);

    // Input address should be hardware address and at byte granularity
    bool update_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size);

private:
    /* Member functions for swapping */
    void initialize_swapping();

    uint8_t operate_swapping();

    // This function is used by memories, like Ramulator.
    void return_swapping_data(ramulator::Request& request);

    // This function is used by memory controller in add_rq() and add_wq().
    uint8_t check_request(request_type& packet, ramulator::Request::Type type); // Packet needs to prepare its hardware address.
    uint8_t check_address(uint64_t address, uint8_t type);                      // The address is physical address.

#endif // MEMORY_USE_SWAPPING_UNIT
};

template<typename T, typename T2>
MEMORY_CONTROLLER<T, T2>::MEMORY_CONTROLLER(double freq_scale, double clock_scale, double clock_scale2, std::vector<channel_type*>&& ul, ramulator::Memory<T, ramulator::Controller>& memory, ramulator::Memory<T2, ramulator::Controller>& memory2)
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
: champsim::operable(freq_scale), clock_scale(clock_scale), clock_scale2(clock_scale2),
  queues(std::move(ul)), memory(memory), memory2(memory2),
  os_transparent_management(*(new OS_TRANSPARENT_MANAGEMENT(memory.max_address + memory2.max_address, memory.max_address)))
#else
: champsim::operable(freq_scale), clock_scale(clock_scale), clock_scale2(clock_scale2),
  queues(std::move(ul)), memory(memory), memory2(memory2)
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
{
    std::printf("clock_scale: %f, clock_scale2: %f.\n", clock_scale, clock_scale2);

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    swapping_count = swapping_traffic_in_bytes = 0;
#endif // MEMORY_USE_SWAPPING_UNIT

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    load_request_in_memory = load_request_in_memory2 = 0;
    store_request_in_memory = store_request_in_memory2 = 0;
#endif // TRACKING_LOAD_STORE_STATISTICS

    read_request_in_memory = read_request_in_memory2 = 0;
    write_request_in_memory = write_request_in_memory2 = 0;

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    initialize_swapping();

#if (TEST_SWAPPING_UNIT == ENABLE)
    for (auto i = 0; i < MEMORY_DATA_NUMBER; i++)
    {
        memory_data[i][0] = i;
    }
#endif // TEST_SWAPPING_UNIT
#endif // MEMORY_USE_SWAPPING_UNIT
}

template<typename T, typename T2>
MEMORY_CONTROLLER<T, T2>::~MEMORY_CONTROLLER()
{
// Print information to output_statistics
#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    output_statistics.swapping_count            = swapping_count;
    output_statistics.swapping_traffic_in_bytes = swapping_traffic_in_bytes;
#endif // MEMORY_USE_SWAPPING_UNIT

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    output_statistics.load_request_in_memory   = load_request_in_memory;
    output_statistics.store_request_in_memory  = store_request_in_memory;
    output_statistics.load_request_in_memory2  = load_request_in_memory2;
    output_statistics.store_request_in_memory2 = store_request_in_memory2;
#endif // TRACKING_LOAD_STORE_STATISTICS

    output_statistics.read_request_in_memory   = read_request_in_memory;
    output_statistics.read_request_in_memory2  = read_request_in_memory2;
    output_statistics.write_request_in_memory  = write_request_in_memory;
    output_statistics.write_request_in_memory2 = write_request_in_memory2;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    delete &os_transparent_management;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
}

template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::initialize()
{
    long long int dram_size = (memory.max_address + memory2.max_address) / MiB; // in MiB

#if (USE_VCPKG == ENABLE)
    fmt::print("Memory Subsystem Size: ");
    if (dram_size > 1024)
    {
        fmt::print("{} GiB", dram_size / 1024);
    }
    else
    {
        fmt::print("{} MiB", dram_size);
    }
    fmt::print("\n");

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "Memory Subsystem Size: ");
    if (dram_size > 1024)
    {
        std::fprintf(output_statistics.file_handler, "%lld GiB", dram_size / 1024);
    }
    else
    {
        std::fprintf(output_statistics.file_handler, "%lld MiB", dram_size);
    }
    std::fprintf(output_statistics.file_handler, "\n");

#endif // PRINT_STATISTICS_INTO_FILE
}

template<typename T, typename T2>
long MEMORY_CONTROLLER<T, T2>::operate()
{
    long progress {0};

    initiate_requests();

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.cold_data_detection();

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    for (size_t i = 0; i < os_transparent_management.incomplete_read_request_queue.size(); i++)
    {
        if (os_transparent_management.incomplete_read_request_queue[i].fm_access_finish) // This read request is ready to access slow memory
        {
            request_type packet              = os_transparent_management.incomplete_read_request_queue[i].packet;

            DRAM_CHANNEL::request_type rq_it = DRAM_CHANNEL::request_type {packet};
            rq_it.forward_checked            = false;
            rq_it.event_cycle                = current_cycle;
            if (packet.response_requested)
                rq_it.to_return = {&(queues.at(packet.cpu)->returned)}; // Store the response queue to communicate with the LLC

            /* Send memory request below */
            bool stall       = true;

            uint64_t address = packet.h_address;
            if ((memory.max_address <= address) && (address < memory.max_address + memory2.max_address))
            {
                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                ramulator::Request request(address - memory.max_address, ramulator::Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), rq_it, packet.cpu, memory2_id);
                stall = ! memory2.send(request);

                if (stall == false)
                {
                    read_request_in_memory2++;
                    os_transparent_management.incomplete_read_request_queue.erase(os_transparent_management.incomplete_read_request_queue.begin() + i);
                }
            }
            else
            {
                std::printf("%s: Error!\n", __FUNCTION__);
            }
        }
    }

    for (size_t i = 0; i < os_transparent_management.incomplete_write_request_queue.size(); i++)
    {
        if (os_transparent_management.incomplete_write_request_queue[i].fm_access_finish) // This write request is ready to write memory (fast or slow)
        {
            request_type packet              = os_transparent_management.incomplete_write_request_queue[i].packet;

            DRAM_CHANNEL::request_type wq_it = DRAM_CHANNEL::request_type {packet};
            wq_it.forward_checked            = false;
            wq_it.event_cycle                = current_cycle;

            /* Send memory request below */
            bool stall                       = true;

            uint64_t address                 = packet.h_address;

            // Assign the request to the right memory.
            if (address < memory.max_address)
            {
                ramulator::Request request(address, ramulator::Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory_id);
                stall = ! memory.send(request);

                if (stall == false)
                {
                    write_request_in_memory++;
                    os_transparent_management.incomplete_write_request_queue.erase(os_transparent_management.incomplete_write_request_queue.begin() + i);
                }
            }
            else if (address < memory.max_address + memory2.max_address)
            {
                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                ramulator::Request request(address - memory.max_address, ramulator::Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory2_id);
                stall = ! memory2.send(request);

                if (stall == false)
                {
                    write_request_in_memory2++;
                    os_transparent_management.incomplete_write_request_queue.erase(os_transparent_management.incomplete_write_request_queue.begin() + i);
                }
            }
            else
            {
                std::printf("%s: Error!\n", __FUNCTION__);
            }
        }
    }
#endif // COLOCATED_LINE_LOCATION_TABLE
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Operate swapping below */
    uint8_t swapping_states = operate_swapping();
    switch (swapping_states)
    {
    case 0: // The swapping unit is idle
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // Get a new remapping request.
        {
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            start_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            start_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD
        }
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
    }
    break;
    case 1: // The swapping unit is busy
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // Get a remapping request.
        {
            // In case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error." << std::endl;
            assert(false);
        }
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
    }
    break;
    case 2: // The swapping unit finishes a swapping request
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        bool is_updated = false;
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // Get a remapping request.
        {
            // In case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            is_updated = update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            is_updated = update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error 2." << std::endl;
            assert(false);
        }

        if (is_updated == false)
        {
            os_transparent_management.finish_remapping_request();
            initialize_swapping();
        }
#else

        initialize_swapping();
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
    }
    break;
    default:
        break;
    }
#else

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    // Since we don't consider data swapping overhead here, the data swapping is finished immediately
    OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
    bool issue = os_transparent_management.issue_remapping_request(remapping_request);
    if (issue == true) // Get a new remapping request.
    {
        os_transparent_management.finish_remapping_request();
    }
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif // MEMORY_USE_SWAPPING_UNIT

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#if (IDEAL_SINGLE_MEMPOD == ENABLE)
    os_transparent_management.check_interval_swap(swapping_states, warmup);
#endif // IDEAL_SINGLE_MEMPOD
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    /* Operate memories below */
    static double leap_operation = 0, leap_operation2 = 0;
    // Skip periodically
    if (leap_operation >= 1)
    {
        leap_operation -= 1;
    }
    else
    {
        memory.tick();
        leap_operation += clock_scale;
    }

    if (leap_operation2 >= 1)
    {
        leap_operation2 -= 1;
    }
    else
    {
        memory2.tick();
        leap_operation2 += clock_scale2;
    }

    Stats::curTick++; // Processor clock, global, for Statistics

    return ++progress;
}

template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::begin_phase()
{
    for (auto ul : queues)
    {
        channel_type::stats_type ul_new_roi_stats, ul_new_sim_stats;
        ul->roi_stats = ul_new_roi_stats;
        ul->sim_stats = ul_new_sim_stats;
    }
}

template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::end_phase(unsigned)
{
    /** No code here */
}

// LCOV_EXCL_START Exclude the following function from LCOV
template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::print_deadlock()
{
    std::printf("MEMORY_CONTROLLER %s.\n", __func__);

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    std::printf("base_address[0]: %ld, base_address[1]: %ld.\n", base_address[0], base_address[1]);
#endif // MEMORY_USE_SWAPPING_UNIT
}

// LCOV_EXCL_STOP

template<typename T, typename T2>
std::size_t MEMORY_CONTROLLER<T, T2>::size() const
{
    return (memory.max_address + memory2.max_address);
}

template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::initiate_requests()
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

template<typename T, typename T2>
bool MEMORY_CONTROLLER<T, T2>::add_rq(request_type& packet, champsim::channel* ul)
{
    const static ramulator::Request::Type type = ramulator::Request::Type::READ; // It means the input request is read request.

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    access_type type_origin = packet.type_origin;
#endif // TRACKING_LOAD_STORE_STATISTICS

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management.memory_activity_tracking(packet.address, type, packet.type_origin, float(get_occupancy(type, packet.address)) / get_size(type, packet.address));
#else
    os_transparent_management.memory_activity_tracking(packet.address, type, float(get_occupancy(type, packet.address)) / get_size(type, packet.address));
#endif // TRACKING_LOAD_STORE_STATISTICS
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(packet, type);
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
#endif // MEMORY_USE_SWAPPING_UNIT

    DRAM_CHANNEL::request_type rq_it = DRAM_CHANNEL::request_type {packet};
    rq_it.forward_checked            = false;
    rq_it.event_cycle                = current_cycle;
    if (packet.response_requested)
        rq_it.to_return = {&ul->returned}; // Store the response queue to communicate with the LLC

    /* Send memory request below */
    bool stall = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = rq_it.h_address = packet.h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool is_sm_request = false; // Whether this request is mapped in slow memory according to the LLT.
    if (memory.max_address <= address)
    {
        is_sm_request = true;
        address = rq_it.h_address_fm = packet.h_address_fm; // Pretend to access the Location Entry and Data (LEAD) in fast memory

        if (memory.max_address <= address)
        {
            std::cout << __func__ << ": co_located LLT error, h_address_fm is uncorrect." << std::endl;
            abort();
        }

        // Check incomplete_read_request_queue's size
        if (os_transparent_management.incomplete_read_request_queue.size() >= INCOMPLETE_READ_REQUEST_QUEUE_LENGTH)
        {
            return false;
        }
    }
#endif // COLOCATED_LINE_LOCATION_TABLE

#else
    address = packet.address;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, type, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), rq_it, packet.cpu, memory_id);
        stall = ! memory.send(request);

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
#endif // TRACKING_LOAD_STORE_STATISTICS

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
            if (is_sm_request)
            {
                read_request_in_memory--;

                OS_TRANSPARENT_MANAGEMENT::ReadRequest read_request;
                // Create new read_request
                read_request.packet           = packet;
                read_request.fm_access_finish = false;

                os_transparent_management.incomplete_read_request_queue.push_back(read_request);
            }
#endif // COLOCATED_LINE_LOCATION_TABLE
        }
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        ramulator::Request request(address - memory.max_address, type, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), rq_it, packet.cpu, memory2_id);
        stall = ! memory2.send(request);

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
#endif // TRACKING_LOAD_STORE_STATISTICS
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // Output memory trace
    output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return true;
    }
}

template<typename T, typename T2>
bool MEMORY_CONTROLLER<T, T2>::add_wq(request_type& packet)
{
    const static ramulator::Request::Type type = ramulator::Request::Type::WRITE; // It means the input request is write request.

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management.memory_activity_tracking(packet.address, type, packet.type_origin, float(get_occupancy(type, packet.address)) / get_size(type, packet.address));
#else
    os_transparent_management.memory_activity_tracking(packet.address, type, float(get_occupancy(type, packet.address)) / get_size(type, packet.address));
#endif // TRACKING_LOAD_STORE_STATISTICS
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(packet, type);
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
#endif // MEMORY_USE_SWAPPING_UNIT

    DRAM_CHANNEL::request_type wq_it = DRAM_CHANNEL::request_type {packet};
    wq_it.forward_checked            = false;
    wq_it.event_cycle                = current_cycle;

    /* Send memory request below */
    bool stall                       = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = wq_it.h_address = packet.h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    // Check incomplete_write_request_queue's size
    if (os_transparent_management.incomplete_write_request_queue.size() >= INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH)
    {
        return false;
    }

    address = wq_it.h_address_fm = packet.h_address_fm; // Pretend to access the Location Entry and Data (LEAD) in fast memory
    ramulator::Request request(address, ramulator::Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory_id);
    stall = ! memory.send(request);

    if (stall == false)
    {
        OS_TRANSPARENT_MANAGEMENT::WriteRequest write_request;
        // Create new write_request
        write_request.packet           = packet;
        write_request.fm_access_finish = false;

        os_transparent_management.incomplete_write_request_queue.push_back(write_request);

        return true;
    }
    else
    {
        return false;
    }
#endif // COLOCATED_LINE_LOCATION_TABLE

#else
    address = packet.address;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, type, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory_id);
        stall = ! memory.send(request);

        if (stall == false)
        {
            write_request_in_memory++;
        }
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        ramulator::Request request(address - memory.max_address, type, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory2_id);
        stall = ! memory2.send(request);

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
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return true;
    }
}

template<class T, class T2>
uint32_t MEMORY_CONTROLLER<T, T2>::get_occupancy(ramulator::Request::Type queue_type, uint64_t address)
{
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(address);
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, queue_type);
        return memory.get_queue_occupancy(request);
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        ramulator::Request request(address - memory.max_address, queue_type);
        return memory2.get_queue_occupancy(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T, class T2>
uint32_t MEMORY_CONTROLLER<T, T2>::get_size(ramulator::Request::Type queue_type, uint64_t address)
{
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(address);
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, queue_type);
        return memory.get_queue_size(request);
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // The memory itself doesn't know other memories' space, so we manage the overall mapping.
        ramulator::Request request(address - memory.max_address, queue_type);
        return memory2.get_queue_size(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<typename T, typename T2>
void MEMORY_CONTROLLER<T, T2>::return_data(ramulator::Request& request)
{
    // Recover the hardware address to physical address.
    switch (request.memory_id)
    {
    case MEMORY_NUMBER_ONE:
        // Nothing to do
        break;
    case MEMORY_NUMBER_TWO:
        request.addr += memory.max_address;
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

    if (uint64_t(request.addr) < memory.max_address)
    {
        // This could be an uncomplete write request
        bool finish = os_transparent_management.finish_fm_access_in_incomplete_write_request_queue(request.packet.h_address);
        if (finish)
        {
            finish_return_data = true;
            return;
        }
    }

    if ((uint64_t(request.addr) < memory.max_address) && (memory.max_address <= request.packet.h_address))
    {
        // This could be an uncomplete read request
        bool finish = os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(request.packet.h_address);

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
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT && COLOCATED_LINE_LOCATION_TABLE
};

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)

// Functions for swapping:
template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::initialize_swapping()
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
template<class T, class T2>
bool MEMORY_CONTROLLER<T, T2>::start_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
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
template<class T, class T2>
bool MEMORY_CONTROLLER<T, T2>::update_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
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

template<class T, class T2>
uint8_t MEMORY_CONTROLLER<T, T2>::operate_swapping()
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
                        if (address < memory.max_address)
                        {
                            ramulator::Request request(address, ramulator::Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_swapping_data, this, placeholders::_1), coreid, memory_id);
                            stall = ! memory.send(request);
                        }
                        else if (address < memory.max_address + memory2.max_address)
                        {
                            // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                            ramulator::Request request(address - memory.max_address, ramulator::Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_swapping_data, this, placeholders::_1), coreid, memory2_id);
                            stall = ! memory2.send(request);
                        }
                        else
                        {
                            std::printf("%s: Error!\n", __FUNCTION__);
                        }

#if (PRINT_MEMORY_TRACE == ENABLE)
                        // Output memory trace.
                        output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif // PRINT_MEMORY_TRACE

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
                            if (address < memory.max_address)
                            {
                                ramulator::Request request(address, ramulator::Request::Type::WRITE, NULL, coreid, memory_id);
                                // Get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory.send(request);
                            }
                            else if (address < memory.max_address + memory2.max_address)
                            {
                                // The memory itself doesn't know other memories' space, so we manage the overall mapping.
                                ramulator::Request request(address - memory.max_address, ramulator::Request::Type::WRITE, NULL, coreid, memory2_id);
                                // Get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory2.send(request);
                            }
                            else
                            {
                                std::printf("%s: Error!\n", __FUNCTION__);
                            }

#if (PRINT_MEMORY_TRACE == ENABLE)
                            // Output memory trace.
                            output_memorytrace.output_memory_trace_hexadecimal(address, 'W');
#endif // PRINT_MEMORY_TRACE

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
#endif // TEST_SWAPPING_UNIT
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
template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::return_swapping_data(ramulator::Request& request)
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
        request.addr += memory.max_address;
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
#endif // TEST_SWAPPING_UNIT
    }
    else if ((base_address[SWAPPING_SEGMENT_TWO] <= address) && (address < (base_address[SWAPPING_SEGMENT_TWO] + active_entry_number)))
    {
        segment_index = SWAPPING_SEGMENT_ONE;
        entry_index   = static_cast<uint8_t>(address - base_address[SWAPPING_SEGMENT_TWO]);

#if (TEST_SWAPPING_UNIT == ENABLE)
        request.data = memory_data[entry_index + MEMORY_DATA_NUMBER / 2];
#endif // TEST_SWAPPING_UNIT
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

template<class T, class T2>
uint8_t MEMORY_CONTROLLER<T, T2>::check_request(request_type& packet, ramulator::Request::Type type)
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

    if (type == ramulator::Request::Type::READ) // For read request
    {
        if ((buffer[entry_index].finish == true) || (buffer[entry_index].read[segment_index] == true) || (buffer[entry_index].write[segment_index] == true) || (buffer[entry_index].dirty[segment_index] == true))
        {
            uint64_t& read_data = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // Note the PACKET only has 64 bit data.
            packet.data         = read_data;

            return 2; // Though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else if (type == ramulator::Request::Type::WRITE) // For write request
    {
        if ((buffer[entry_index].read[0] == true) && (buffer[entry_index].read[1] == true))
        {
            uint64_t& write_data                     = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // Note the PACKET only has 64 bit data.
            write_data                               = packet.data;
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

template<class T, class T2>
uint8_t MEMORY_CONTROLLER<T, T2>::check_address(uint64_t address, uint8_t type)
{
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(address);
#else
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

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

#endif // MEMORY_USE_SWAPPING_UNIT

#else

template<typename T>
class MEMORY_CONTROLLER : public champsim::operable
{
    double clock_scale  = MEMORY_CONTROLLER_CLOCK_SCALE;

    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    void initiate_requests();
    bool add_rq(const request_type& pkt, champsim::channel* ul);
    bool add_wq(const request_type& pkt);

public:
    // Note here they are the references to escape memory deallocation here.
    ramulator::Memory<T, ramulator::Controller>& memory;

    const uint8_t memory_id = 0;

    uint64_t read_request_in_memory;
    uint64_t write_request_in_memory;

    /* Member functions */
    MEMORY_CONTROLLER(double freq_scale, double clock_scale, std::vector<channel_type*>&& ul, ramulator::Memory<T, ramulator::Controller>& memory);
    ~MEMORY_CONTROLLER();

    void initialize() override final;
    long operate() override final;
    void begin_phase() override final;
    void end_phase(unsigned cpu) override final;
    void print_deadlock() override final;

    std::size_t size() const;

    /** @brief
     *  Get the number of valid members in the related write/read queue.
     *  The address is physical address.
     */
    uint32_t get_occupancy(ramulator::Request::Type queue_type, uint64_t address);

    /** @brief
     *  Get the capacity of the related write/read queue.
     *  The address is physical address.
     */
    uint32_t get_size(ramulator::Request::Type queue_type, uint64_t address);

    void return_data(ramulator::Request& request);
};

template<typename T>
MEMORY_CONTROLLER<T>::MEMORY_CONTROLLER(double freq_scale, double clock_scale, std::vector<channel_type*>&& ul, ramulator::Memory<T, ramulator::Controller>& memory)
: champsim::operable(freq_scale), clock_scale(clock_scale),
  queues(std::move(ul)), memory(memory)
{
    std::printf("clock_scale: %f.\n", clock_scale);

    read_request_in_memory  = 0;
    write_request_in_memory = 0;
}

template<typename T>
MEMORY_CONTROLLER<T>::~MEMORY_CONTROLLER()
{
    // Print information to output_statistics
    output_statistics.read_request_in_memory  = read_request_in_memory;
    output_statistics.write_request_in_memory = write_request_in_memory;
}

template<typename T>
void MEMORY_CONTROLLER<T>::initialize()
{
    long long int dram_size = memory.max_address / MiB; // in MiB

#if (USE_VCPKG == ENABLE)
    fmt::print("Memory Subsystem Size: ");
    if (dram_size > 1024)
    {
        fmt::print("{} GiB", dram_size / 1024);
    }
    else
    {
        fmt::print("{} MiB", dram_size);
    }
    fmt::print("\n");

#endif // USE_VCPKG

#if (PRINT_STATISTICS_INTO_FILE == ENABLE)
    std::fprintf(output_statistics.file_handler, "Memory Subsystem Size: ");
    if (dram_size > 1024)
    {
        std::fprintf(output_statistics.file_handler, "%lld GiB", dram_size / 1024);
    }
    else
    {
        std::fprintf(output_statistics.file_handler, "%lld MiB", dram_size);
    }
    std::fprintf(output_statistics.file_handler, "\n");

#endif // PRINT_STATISTICS_INTO_FILE
}

template<typename T>
long MEMORY_CONTROLLER<T>::operate()
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
        memory.tick();
        leap_operation += clock_scale;
    }

    Stats::curTick++; // Processor clock, global, for Statistics

    return ++progress;
}

template<typename T>
void MEMORY_CONTROLLER<T>::begin_phase()
{
    for (auto ul : queues)
    {
        channel_type::stats_type ul_new_roi_stats, ul_new_sim_stats;
        ul->roi_stats = ul_new_roi_stats;
        ul->sim_stats = ul_new_sim_stats;
    }
}

template<typename T>
void MEMORY_CONTROLLER<T>::end_phase(unsigned)
{
    /** No code here */
}

// LCOV_EXCL_START Exclude the following function from LCOV
template<typename T>
void MEMORY_CONTROLLER<T>::print_deadlock()
{
    std::printf("MEMORY_CONTROLLER %s.\n", __func__);
}

// LCOV_EXCL_STOP

template<typename T>
std::size_t MEMORY_CONTROLLER<T>::size() const
{
    return memory.max_address;
}

template<typename T>
void MEMORY_CONTROLLER<T>::initiate_requests()
{
    // Initiate read requests
    for (auto ul : queues)
    {
        for (auto q : {std::ref(ul->RQ), std::ref(ul->PQ)})
        {
            auto [begin, end] = champsim::get_span_p(std::cbegin(q.get()), std::cend(q.get()), [ul, this](const auto& pkt)
                { return this->add_rq(pkt, ul); }); // Add read requests
            q.get().erase(begin, end);
        }

        // Initiate write requests
        auto [wq_begin, wq_end] = champsim::get_span_p(std::cbegin(ul->WQ), std::cend(ul->WQ), [this](const auto& pkt)
            { return this->add_wq(pkt); }); // Add write requests
        ul->WQ.erase(wq_begin, wq_end);
    }
}

template<typename T>
bool MEMORY_CONTROLLER<T>::add_rq(const request_type& packet, champsim::channel* ul)
{
    const static ramulator::Request::Type type = ramulator::Request::Type::READ; // It means the input request is read request.

    DRAM_CHANNEL::request_type rq_it           = DRAM_CHANNEL::request_type {packet};
    rq_it.forward_checked                      = false;
    rq_it.event_cycle                          = current_cycle;
    if (packet.response_requested)
        rq_it.to_return = {&ul->returned}; // Store the response queue to communicate with the LLC

    /* Send memory request below */
    bool stall       = true;

    uint64_t address = packet.address;

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, type, std::bind(&MEMORY_CONTROLLER<T>::return_data, this, placeholders::_1), rq_it, packet.cpu, memory_id);
        stall = ! memory.send(request);

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
    // Output memory trace
    output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return true;
    }
}

template<typename T>
bool MEMORY_CONTROLLER<T>::add_wq(const request_type& packet)
{
    const static ramulator::Request::Type type = ramulator::Request::Type::WRITE; // It means the input request is write request.

    DRAM_CHANNEL::request_type wq_it           = DRAM_CHANNEL::request_type {packet};
    wq_it.forward_checked                      = false;
    wq_it.event_cycle                          = current_cycle;

    /* Send memory request below */
    bool stall                                 = true;

    uint64_t address                           = packet.address;

    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, type, std::bind(&MEMORY_CONTROLLER<T>::return_data, this, placeholders::_1), wq_it, packet.cpu, memory_id);
        stall = ! memory.send(request);

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
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return false; // Queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return true;
    }
}

template<class T>
uint32_t MEMORY_CONTROLLER<T>::get_occupancy(ramulator::Request::Type queue_type, uint64_t address)
{
    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, queue_type);
        return memory.get_queue_occupancy(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T>
uint32_t MEMORY_CONTROLLER<T>::get_size(ramulator::Request::Type queue_type, uint64_t address)
{
    // Assign the request to the right memory.
    if (address < memory.max_address)
    {
        ramulator::Request request(address, queue_type);
        return memory.get_queue_size(request);
    }
    else
    {
        std::printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<typename T>
void MEMORY_CONTROLLER<T>::return_data(ramulator::Request& request)
{
    response_type response {request.packet.address, request.packet.v_address, request.packet.data, request.packet.pf_metadata, request.packet.instr_depend_on_me};

    for (auto ret : request.packet.to_return)
    {
        ret->push_back(response); // Fill the response into the response queue
    }
};

#endif // MEMORY_USE_HYBRID

#else

struct dram_stats
{
    std::string name {};
    uint64_t dbus_cycle_congested = 0, dbus_count_congested = 0;

    unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
};

struct DRAM_CHANNEL
{
    using response_type = typename champsim::channel::response_type;

    struct request_type
    {
        bool scheduled       = false;
        bool forward_checked = false;

        uint8_t asid[2]      = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};

        uint32_t pf_metadata = 0;

        uint64_t address     = 0;
        uint64_t v_address   = 0;
        uint64_t data        = 0;
        uint64_t event_cycle = std::numeric_limits<uint64_t>::max();

        std::vector<std::reference_wrapper<ooo_model_instr> > instr_depend_on_me {};
        std::vector<std::deque<response_type>*> to_return {};

        explicit request_type(typename champsim::channel::request_type);
    };

    using value_type = request_type;
    using queue_type = std::vector<std::optional<value_type> >;
    queue_type WQ {DRAM_WQ_SIZE}, RQ {DRAM_RQ_SIZE};

    struct BANK_REQUEST
    {
        bool valid = false, row_buffer_hit = false;

        std::size_t open_row = std::numeric_limits<uint32_t>::max();

        uint64_t event_cycle = 0;

        queue_type::iterator pkt;
    };

    using request_array_type                    = std::array<BANK_REQUEST, DRAM_RANKS * DRAM_BANKS>;
    request_array_type bank_request             = {};
    request_array_type::iterator active_request = std::end(bank_request);

    bool write_mode                             = false;
    uint64_t dbus_cycle_available               = 0;

    using stats_type                            = dram_stats;
    stats_type roi_stats, sim_stats;

    void check_collision();
    void print_deadlock();
};

class MEMORY_CONTROLLER : public champsim::operable
{
    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    /** @note
     *  DRAM_IO_FREQ defined in champsim_constants.h
     *
     *  The unit of tRP/tRCD/tCAS/DRAM_DBUS_TURN_AROUND_TIME is cycle (1 cycle = 1 / DRAM_IO_FREQ microsecond)
     *  tRP is the time of precharging phase.
     *
     *  tRCD is row-to-column delay, which is the time when the subarray copies the row into the sense-amplifier
     *  after receiving ACTIVATE command from the DRAM controller.
     *
     *  tCAS is the time between sending a column address to the memory and the beginning of the data in response, or,
     *  is the delay, in clock cycles, between the internal READ command and the availability of the first bit of output
     *  data.
     *  In other words, tCAS is tCL which means the time between the data leaves the subarray and reaches the processor.
     *  Also, tCL is the abbreviation of CAS latency (CL).
     *
     *  CAS WRITE latency (CWL) is the delay, in clock cycles, between the internal WRITE command and the availability of
     *  the first bit of output data.
     *
     *  Thus, READ latency (RL) is controlled by the sum of the tRCD and tCAS register settings.
     *  WRITE latency (WL) is controlled by the sum of the tRCD and tCWL register settings.
     *  [reference](https://doi.org/10.1109/HPCA.2013.6522354)
     *  [reference](https://www.micron.com/-/media/client/global/documents/products/data-sheet/ddr4/8gb_ddr4_sdram.pdf, Micron 8Gb: x4, x8, x16 DDR4 SDRAM Features)
     */
    // Latencies
    const uint64_t tRP, tRCD, tCAS, DRAM_DBUS_TURN_AROUND_TIME, DRAM_DBUS_RETURN_TIME;

    // These values control when to send out a burst of writes
    constexpr static std::size_t DRAM_WRITE_HIGH_WM         = ((DRAM_WQ_SIZE * 7) >> 3); // 7/8th
    constexpr static std::size_t DRAM_WRITE_LOW_WM          = ((DRAM_WQ_SIZE * 6) >> 3); // 6/8th
    constexpr static std::size_t MIN_DRAM_WRITES_PER_SWITCH = ((DRAM_WQ_SIZE * 1) >> 2); // 1/4

    void initiate_requests();
    bool add_rq(const request_type& pkt, champsim::channel* ul);
    bool add_wq(const request_type& pkt);

public:
    std::array<DRAM_CHANNEL, DRAM_CHANNELS> channels;

    MEMORY_CONTROLLER(double freq_scale, int io_freq, double t_rp, double t_rcd, double t_cas, double turnaround, std::vector<channel_type*>&& ul);

    void initialize() override final;
    long operate() override final;
    void begin_phase() override final;
    void end_phase(unsigned cpu) override final;
    void print_deadlock() override final;

    std::size_t size() const;

    uint32_t dram_get_channel(uint64_t address);
    uint32_t dram_get_rank(uint64_t address);
    uint32_t dram_get_bank(uint64_t address);
    uint32_t dram_get_row(uint64_t address);
    uint32_t dram_get_column(uint64_t address);
};

#endif // RAMULATOR

#else
/* Original code of ChampSim */

struct dram_stats
{
    std::string name {};
    uint64_t dbus_cycle_congested = 0, dbus_count_congested = 0;

    unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
};

struct DRAM_CHANNEL
{
    using response_type = typename champsim::channel::response_type;

    struct request_type
    {
        bool scheduled       = false;
        bool forward_checked = false;

        uint8_t asid[2]      = {std::numeric_limits<uint8_t>::max(), std::numeric_limits<uint8_t>::max()};

        uint32_t pf_metadata = 0;

        uint64_t address     = 0;
        uint64_t v_address   = 0;
        uint64_t data        = 0;
        uint64_t event_cycle = std::numeric_limits<uint64_t>::max();

        std::vector<std::reference_wrapper<ooo_model_instr> > instr_depend_on_me {};
        std::vector<std::deque<response_type>*> to_return {};

        explicit request_type(typename champsim::channel::request_type);
    };

    using value_type = request_type;
    using queue_type = std::vector<std::optional<value_type> >;
    queue_type WQ {DRAM_WQ_SIZE}, RQ {DRAM_RQ_SIZE};

    struct BANK_REQUEST
    {
        bool valid = false, row_buffer_hit = false;

        std::size_t open_row = std::numeric_limits<uint32_t>::max();

        uint64_t event_cycle = 0;

        queue_type::iterator pkt;
    };

    using request_array_type                    = std::array<BANK_REQUEST, DRAM_RANKS * DRAM_BANKS>;
    request_array_type bank_request             = {};
    request_array_type::iterator active_request = std::end(bank_request);

    bool write_mode                             = false;
    uint64_t dbus_cycle_available               = 0;

    using stats_type                            = dram_stats;
    stats_type roi_stats, sim_stats;

    void check_collision();
    void print_deadlock();
};

class MEMORY_CONTROLLER : public champsim::operable
{
    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    // Latencies
    const uint64_t tRP, tRCD, tCAS, DRAM_DBUS_TURN_AROUND_TIME, DRAM_DBUS_RETURN_TIME;

    // these values control when to send out a burst of writes
    constexpr static std::size_t DRAM_WRITE_HIGH_WM         = ((DRAM_WQ_SIZE * 7) >> 3); // 7/8th
    constexpr static std::size_t DRAM_WRITE_LOW_WM          = ((DRAM_WQ_SIZE * 6) >> 3); // 6/8th
    constexpr static std::size_t MIN_DRAM_WRITES_PER_SWITCH = ((DRAM_WQ_SIZE * 1) >> 2); // 1/4

    void initiate_requests();
    bool add_rq(const request_type& pkt, champsim::channel* ul);
    bool add_wq(const request_type& pkt);

public:
    std::array<DRAM_CHANNEL, DRAM_CHANNELS> channels;

    MEMORY_CONTROLLER(double freq_scale, int io_freq, double t_rp, double t_rcd, double t_cas, double turnaround, std::vector<channel_type*>&& ul);

    void initialize() override final;
    long operate() override final;
    void begin_phase() override final;
    void end_phase(unsigned cpu) override final;
    void print_deadlock() override final;

    std::size_t size() const;

    uint32_t dram_get_channel(uint64_t address);
    uint32_t dram_get_rank(uint64_t address);
    uint32_t dram_get_bank(uint64_t address);
    uint32_t dram_get_row(uint64_t address);
    uint32_t dram_get_column(uint64_t address);
};

#endif // USER_CODES

    /* Variable */

    /* Function */

#endif

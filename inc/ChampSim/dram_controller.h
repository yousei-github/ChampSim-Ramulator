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

#ifndef DRAM_H
#define DRAM_H

#include <array>
#include <cmath>
#include <limits>

#include "ProjectConfiguration.h" // user file
#include "champsim_constants.h"
#include "memory_class.h"
#include "operable.h"
#include "os_transparent_management.h"
#include "util.h"

#if (RAMULATOR == ENABLE)
#else
// these values control when to send out a burst of writes
constexpr std::size_t DRAM_WRITE_HIGH_WM         = ((DRAM_WQ_SIZE * 7) >> 3); // 7/8th
constexpr std::size_t DRAM_WRITE_LOW_WM          = ((DRAM_WQ_SIZE * 6) >> 3); // 6/8th
constexpr std::size_t MIN_DRAM_WRITES_PER_SWITCH = ((DRAM_WQ_SIZE * 1) >> 2); // 1/4
#endif // RAMULATOR

namespace detail
{
// https://stackoverflow.com/a/31962570
constexpr int32_t ceil(float num)
{
    return (static_cast<float>(static_cast<int32_t>(num)) == num) ? static_cast<int32_t>(num) : static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
}
} // namespace detail

#if (USER_CODES == ENABLE)
#include <cassert>
#include <cstdint>
#include <iostream>

#if (RAMULATOR == ENABLE)
#include "Memory.h"
#include "Request.h"
using namespace ramulator;
extern uint8_t all_warmup_complete;

#else
struct MEMORY_STATISTICS
{
    uint64_t memory_request_total_service_time = 0, total_issued_memory_request_number = 0;
    float average_memory_access_time = 0;
};

struct BANK_REQUEST
{
    bool valid = false, row_buffer_hit = false;

    // the row address will be stored here when the row is open.
    std::size_t open_row = std::numeric_limits<uint32_t>::max();

    uint64_t event_cycle = 0;

    // store the related PACKET to access
    std::vector<PACKET>::iterator pkt;
};

// DDR and HBM share a flat address space and the address space starts at HBM.
typedef enum
{
    HBM = 0,
    DDR,
} MemoryType;

// Note channels have their own queues, so please be careful when swapping data between different channels.
struct HBM_CHANNEL
{
    std::vector<PACKET> WQ {DRAM_WQ_SIZE};
    std::vector<PACKET> RQ {DRAM_RQ_SIZE};

    std::array<BANK_REQUEST, HBM_BANKS> bank_request             = {};
    std::array<BANK_REQUEST, HBM_BANKS>::iterator active_request = std::end(bank_request);

    uint64_t dbus_cycle_available = 0, dbus_cycle_congested = 0, dbus_count_congested = 0;

    bool write_mode            = false;

    unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
};

struct DDR_CHANNEL
{
    std::vector<PACKET> WQ {DRAM_WQ_SIZE};
    std::vector<PACKET> RQ {DRAM_RQ_SIZE};

    std::array<BANK_REQUEST, DDR_RANKS* DDR_BANKS> bank_request             = {};
    std::array<BANK_REQUEST, DDR_RANKS* DDR_BANKS>::iterator active_request = std::end(bank_request);

    uint64_t dbus_cycle_available = 0, dbus_cycle_congested = 0, dbus_count_congested = 0;

    bool write_mode            = false;

    unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
};
#endif // RAMULATOR

// Define the MEMORY_CONTROLLER class
#if (RAMULATOR == ENABLE)
#if (MEMORY_USE_HYBRID == ENABLE)
template<class T, class T2>
class MEMORY_CONTROLLER : public champsim::operable, public MemoryRequestConsumer
{
public:
    /* General part */
    double clock_scale  = MEMORY_CONTROLLER_CLOCK_SCALE;
    double clock_scale2 = MEMORY_CONTROLLER_CLOCK_SCALE;

    // Note here they are the references to escape memory deallocation here.
    ramulator::Memory<T, Controller>& memory;
    ramulator::Memory<T2, Controller>& memory2;

    const uint8_t memory_id  = MEMORY_NUMBER_ONE;
    const uint8_t memory2_id = MEMORY_NUMBER_TWO;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    OS_TRANSPARENT_MANAGEMENT& os_transparent_management;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Swapping unit */
    struct BUFFER_ENTRY
    {
        bool finish                                                   = false; // whether a swapping of this entry is completed.

        std::array<uint8_t, BLOCK_SIZE> data[SWAPPING_SEGMENT_NUMBER] = {0}; // cache lines
        bool read_issue[SWAPPING_SEGMENT_NUMBER];                            // whether a read request is issued
        bool read[SWAPPING_SEGMENT_NUMBER];                                  // whether a read request is finished
        bool write[SWAPPING_SEGMENT_NUMBER];                                 // whether a write request is finished
        bool dirty[SWAPPING_SEGMENT_NUMBER];                                 // whether a "new" write request is received
    };

    std::array<BUFFER_ENTRY, SWAPPING_BUFFER_ENTRY_NUMBER> buffer = {};
    uint64_t base_address[SWAPPING_SEGMENT_NUMBER]; // base_address[0] for segment 1, base_address[1] for segment 2. Address is hardware address and at cache line granularity.
    uint8_t active_entry_number;
    uint8_t finish_number;

    // scoped enumerations
    enum class SwappingState : uint8_t {
        Idle, Swapping};
    SwappingState states; // the state of swapping mechanism

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

    uint64_t swapping_count;
    uint64_t swapping_traffic_in_bytes;

    /* Member functions */
    MEMORY_CONTROLLER(double freq_scale, double clock_scale, double clock_scale2, Memory<T, Controller>& memory, Memory<T2, Controller>& memory2);
    ~MEMORY_CONTROLLER();

    void operate() override;
    void print_deadlock() override;

    int add_rq(PACKET* packet) override;
    int add_wq(PACKET* packet) override;
    int add_pq(PACKET* packet) override;

    /** @brief
     *  Get the number of valid(active) member in the write/read queue.
     *  The address is physical address.
     */
    uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;

    /** @brief
     *  Get the capacity of the write/read queue.
     *  The address is physical address.
     */
    uint32_t get_size(uint8_t queue_type, uint64_t address) override;

    void return_data(Request& request);

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
public:
    // input address should be hardware address and at byte granularity
    bool start_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size);

    // input address should be hardware address and at byte granularity
    bool update_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size);

private:
    /* Member functions for swapping */
    void initialize_swapping();

    uint8_t operate_swapping();

    // this function is used by memories, like Ramulator.
    void return_swapping_data(Request& request);

    // this function is used by memory controller in add_rq() and add_wq().
    uint8_t check_request(PACKET& packet, uint8_t type);   // packet needs to prepare its hardware address.
    uint8_t check_address(uint64_t address, uint8_t type); // The address is physical address.

#endif // MEMORY_USE_SWAPPING_UNIT
};

template<class T, class T2>
MEMORY_CONTROLLER<T, T2>::MEMORY_CONTROLLER(double freq_scale, double clock_scale, double clock_scale2, Memory<T, Controller>& memory, Memory<T2, Controller>& memory2)
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
: champsim::operable(freq_scale), MemoryRequestConsumer(std::numeric_limits<unsigned>::max()),
  clock_scale(clock_scale), clock_scale2(clock_scale2), memory(memory), memory2(memory2),
  os_transparent_management(*(new OS_TRANSPARENT_MANAGEMENT(memory.max_address + memory2.max_address, memory.max_address)))
#else
: champsim::operable(freq_scale), MemoryRequestConsumer(std::numeric_limits<unsigned>::max()),
  clock_scale(clock_scale), clock_scale2(clock_scale2), memory(memory), memory2(memory2)
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
{
    printf("clock_scale: %f, clock_scale2: %f.\n", clock_scale, clock_scale2);

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    load_request_in_memory = load_request_in_memory2 = 0;
    store_request_in_memory = store_request_in_memory2 = 0;
#endif // TRACKING_LOAD_STORE_STATISTICS

    read_request_in_memory = read_request_in_memory2 = 0;
    write_request_in_memory = write_request_in_memory2 = 0;

    swapping_count = swapping_traffic_in_bytes = 0;

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    initialize_swapping();

#if (TEST_SWAPPING_UNIT == ENABLE)
    for (auto i = 0; i < MEMORY_DATA_NUMBER; i++)
    {
        memory_data[i][0] = i;
    }
#endif // TEST_SWAPPING_UNIT
#endif // MEMORY_USE_SWAPPING_UNIT
};

template<class T, class T2>
MEMORY_CONTROLLER<T, T2>::~MEMORY_CONTROLLER()
{
    // print some information to output_statistics
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    output_statistics.load_request_in_memory   = load_request_in_memory;
    output_statistics.store_request_in_memory  = store_request_in_memory;
    output_statistics.load_request_in_memory2  = load_request_in_memory2;
    output_statistics.store_request_in_memory2 = store_request_in_memory2;
#endif // TRACKING_LOAD_STORE_STATISTICS
    output_statistics.read_request_in_memory    = read_request_in_memory;
    output_statistics.read_request_in_memory2   = read_request_in_memory2;
    output_statistics.write_request_in_memory   = write_request_in_memory;
    output_statistics.write_request_in_memory2  = write_request_in_memory2;

    output_statistics.swapping_count            = swapping_count;
    output_statistics.swapping_traffic_in_bytes = swapping_traffic_in_bytes;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    delete &os_transparent_management;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
};

template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::operate()
{
    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.cold_data_detection();

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    for (size_t i = 0; i < os_transparent_management.incomplete_read_request_queue.size(); i++)
    {
        if (os_transparent_management.incomplete_read_request_queue[i].fm_access_finish) // this read request is ready to access slow memory
        {
            PACKET packet    = os_transparent_management.incomplete_read_request_queue[i].packet;
            /* Send memory request below */
            bool stall       = true;
            uint64_t address = packet.h_address;
            if ((memory.max_address <= address) && (address < memory.max_address + memory2.max_address))
            {
                // the memory itself doesn't know other memories' space, so we manage the overall mapping.
                Request request(address - memory.max_address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), packet, packet.cpu, memory2_id);
                stall = ! memory2.send(request);

                if (stall == false)
                {
                    read_request_in_memory2++;
                    os_transparent_management.incomplete_read_request_queue.erase(os_transparent_management.incomplete_read_request_queue.begin() + i);
                }
            }
            else
            {
                printf("%s: Error!\n", __FUNCTION__);
            }
        }
    }

    for (size_t i = 0; i < os_transparent_management.incomplete_write_request_queue.size(); i++)
    {
        if (os_transparent_management.incomplete_write_request_queue[i].fm_access_finish) // this write request is ready to write memory (fast or slow)
        {
            PACKET packet    = os_transparent_management.incomplete_write_request_queue[i].packet;
            /* Send memory request below */
            bool stall       = true;
            uint64_t address = packet.h_address;

            // assign the request to the right memory.
            if (address < memory.max_address)
            {
                Request request(address, Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), packet, packet.cpu, memory_id);
                stall = ! memory.send(request);

                if (stall == false)
                {
                    write_request_in_memory++;
                    os_transparent_management.incomplete_write_request_queue.erase(os_transparent_management.incomplete_write_request_queue.begin() + i);
                }
            }
            else if (address < memory.max_address + memory2.max_address)
            {
                // the memory itself doesn't know other memories' space, so we manage the overall mapping.
                Request request(address - memory.max_address, Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), packet, packet.cpu, memory2_id);
                stall = ! memory2.send(request);

                if (stall == false)
                {
                    write_request_in_memory2++;
                    os_transparent_management.incomplete_write_request_queue.erase(os_transparent_management.incomplete_write_request_queue.begin() + i);
                }
            }
            else
            {
                printf("%s: Error!\n", __FUNCTION__);
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
    case 0: // the swapping unit is idle
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // get a new remapping request.
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
    case 1: // the swapping unit is busy
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // get a remapping request.
        {
            // in case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error." << std::endl;
            assert(0);
        }
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
    }
    break;
    case 2: // the swapping unit finishes a swapping request
    {
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
        bool is_updated = false;
        OS_TRANSPARENT_MANAGEMENT::RemappingRequest remapping_request;
        bool issue = os_transparent_management.issue_remapping_request(remapping_request);
        if (issue == true) // get a remapping request.
        {
            // in case the swapping segments are updated
#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE) || (IDEAL_VARIABLE_GRANULARITY == ENABLE)
            is_updated = update_swapping_segments(remapping_request.address_in_fm, remapping_request.address_in_sm, remapping_request.size);
#elif (IDEAL_SINGLE_MEMPOD == ENABLE)
            is_updated = update_swapping_segments(remapping_request.h_address_in_fm, remapping_request.h_address_in_sm, remapping_request.size);
#endif // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE, IDEAL_SINGLE_MEMPOD
        }
        else
        {
            std::cout << __func__ << ": issue_remapping_request error 2." << std::endl;
            assert(0);
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
    if (issue == true) // get a new remapping request.
    {
        os_transparent_management.finish_remapping_request();
    }
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif // MEMORY_USE_SWAPPING_UNIT

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#if (IDEAL_SINGLE_MEMPOD == ENABLE)
    os_transparent_management.check_interval_swap(swapping_states);
#endif // IDEAL_SINGLE_MEMPOD
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    /* Operate memories below */
    static double leap_operation = 0, leap_operation2 = 0;
    // skip periodically
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
    Stats::curTick++; // processor clock, global, for Statistics
};

template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::print_deadlock()
{
#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    printf("base_address[0]: %ld, base_address[1]: %ld.\n", base_address[0], base_address[1]);
#endif // MEMORY_USE_SWAPPING_UNIT
};

template<class T, class T2>
int MEMORY_CONTROLLER<T, T2>::add_rq(PACKET* packet)
{
    const static uint8_t type = 1; // it means the input request is read request.

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    uint64_t type_origin = packet->type_origin;
#endif // TRACKING_LOAD_STORE_STATISTICS

    if (all_warmup_complete < NUM_CPUS)
    {
        for (auto ret : packet->to_return)
            ret->return_data(packet);

        return int(ReturnValue::Forward); // Fast-forward
    }

    /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(*packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management.memory_activity_tracking(packet->address, type, packet->type_origin, float(get_occupancy(type, packet->address)) / get_size(type, packet->address));
#else
    os_transparent_management.memory_activity_tracking(packet->address, type, float(get_occupancy(type, packet->address)) / get_size(type, packet->address));
#endif // TRACKING_LOAD_STORE_STATISTICS
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(*packet, type);
    switch (under_swapping)
    {
    case 0:                            // this address is under swapping.
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
        break;
    case 1: // this address is not under swapping.
        break;
    case 2: // though this address is under swapping, we can service its request because the data is in the swapping buffer.
    {
        for (auto ret : packet->to_return)
            ret->return_data(packet);

        return int(ReturnValue::Forward); // Fast-forward
    }
    break;
    default:
        break;
    }
#endif // MEMORY_USE_SWAPPING_UNIT

    /* Send memory request below */
    bool stall = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = packet->h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool is_sm_request = false; // whether this request is mapped in slow memory according to the LLT.
    if (memory.max_address <= address)
    {
        is_sm_request = true;
        address       = packet->h_address_fm; // pretend to access the Location Entry and Data (LEAD) in fast memory

        if (memory.max_address <= address)
        {
            std::cout << __func__ << ": co_located LLT error, h_address_fm is uncorrect." << std::endl;
            abort();
        }

        // check incomplete_read_request_queue's size
        if (os_transparent_management.incomplete_read_request_queue.size() >= INCOMPLETE_READ_REQUEST_QUEUE_LENGTH)
        {
            return int(ReturnValue::Full);
        }
    }
#endif // COLOCATED_LINE_LOCATION_TABLE

#else
    address = packet->address;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), *packet, packet->cpu, memory_id);
        stall = ! memory.send(request);

        if (stall == false)
        {
            read_request_in_memory++;

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
            if (all_warmup_complete > NUM_CPUS)
            {
                if (type_origin == LOAD || type_origin == TRANSLATION)
                {
                    load_request_in_memory++;
                }
                else if (type_origin == RFO)
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
                // create new read_request
                read_request.packet           = *packet;
                read_request.fm_access_finish = false;

                os_transparent_management.incomplete_read_request_queue.push_back(read_request);
            }
#endif // COLOCATED_LINE_LOCATION_TABLE
        }
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // the memory itself doesn't know other memories' space, so we manage the overall mapping.
        Request request(address - memory.max_address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), *packet, packet->cpu, memory2_id);
        stall = ! memory2.send(request);

        if (stall == false)
        {
            read_request_in_memory2++;
        }

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
        if (type_origin == LOAD || type_origin == TRANSLATION)
        {
            load_request_in_memory2++;
        }
        else if (type_origin == RFO)
        {
            store_request_in_memory2++;
        }
        else
        {
            printf("%s: Error!\n", __FUNCTION__);
        }
#endif // TRACKING_LOAD_STORE_STATISTICS
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // output memory trace.
    output_memorytrace.output_memory_trace_hexadecimal(packet->address, 'R');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return get_occupancy(type, packet->address);
    }
};

template<class T, class T2>
int MEMORY_CONTROLLER<T, T2>::add_wq(PACKET* packet)
{
    const static uint8_t type = 2; // it means the input request is write request.

    if (all_warmup_complete < NUM_CPUS)
        return int(ReturnValue::Forward); // Fast-forward

        /* Operate research proposals below */
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(*packet);
#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    os_transparent_management.memory_activity_tracking(packet->address, type, packet->type_origin, float(get_occupancy(type, packet->address)) / get_size(type, packet->address));
#else
    os_transparent_management.memory_activity_tracking(packet->address, type, float(get_occupancy(type, packet->address)) / get_size(type, packet->address));
#endif // TRACKING_LOAD_STORE_STATISTICS
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
    /* Check swapping below */
    uint8_t under_swapping = check_request(*packet, type);
    switch (under_swapping)
    {
    case 0:                            // this address is under swapping.
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
        break;
    case 1: // this address is not under swapping.
        break;
    case 2: // though this address is under swapping, we can service its request because the data is in the swapping buffer.
    {
        return int(ReturnValue::Forward); // Fast-forward
    }
    break;
    default:
        break;
    }
#endif // MEMORY_USE_SWAPPING_UNIT

    /* Send memory request below */
    bool stall = true;

    uint64_t address;
#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    address = packet->h_address;
#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    // check incomplete_write_request_queue's size
    if (os_transparent_management.incomplete_write_request_queue.size() >= INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH)
    {
        return int(ReturnValue::Full);
    }

    address = packet->h_address_fm; // pretend to access the Location Entry and Data (LEAD) in fast memory
    Request request(address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), *packet, packet->cpu, memory_id);
    stall = ! memory.send(request);

    if (stall == false)
    {
        OS_TRANSPARENT_MANAGEMENT::WriteRequest write_request;
        // create new write_request
        write_request.packet           = *packet;
        write_request.fm_access_finish = false;

        os_transparent_management.incomplete_write_request_queue.push_back(write_request);

        return get_occupancy(type, packet->address);
    }
    else
    {
        return int(ReturnValue::Full);
    }
#endif // COLOCATED_LINE_LOCATION_TABLE

#else
    address = packet->address;
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), *packet, packet->cpu, memory_id);
        stall = ! memory.send(request);

        if (stall == false)
        {
            write_request_in_memory++;
        }
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // the memory itself doesn't know other memories' space, so we manage the overall mapping.
        Request request(address - memory.max_address, Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T, T2>::return_data, this, placeholders::_1), *packet, packet->cpu, memory2_id);
        stall = ! memory2.send(request);

        if (stall == false)
        {
            write_request_in_memory2++;
        }
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // output memory trace.
    output_memorytrace.output_memory_trace_hexadecimal(packet->address, 'W');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return get_occupancy(type, packet->address);
    }
};

template<class T, class T2>
int MEMORY_CONTROLLER<T, T2>::add_pq(PACKET* packet)
{
    return add_rq(packet);
};

template<class T, class T2>
uint32_t MEMORY_CONTROLLER<T, T2>::get_occupancy(uint8_t queue_type, uint64_t address)
{
    Request::Type type = Request::Type::READ;

    if (queue_type == 1)
        type = Request::Type::READ;
    else if (queue_type == 2)
        type = Request::Type::WRITE;
    else if (queue_type == 3)
        return get_occupancy(1, address);

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(address);
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, type);
        return memory.get_queue_occupancy(request);
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // the memory itself doesn't know other memories' space, so we manage the overall mapping.
        Request request(address - memory.max_address, type);
        return memory2.get_queue_occupancy(request);
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T, class T2>
uint32_t MEMORY_CONTROLLER<T, T2>::get_size(uint8_t queue_type, uint64_t address)
{
    Request::Type type = Request::Type::READ;

    if (queue_type == 1)
        type = Request::Type::READ;
    else if (queue_type == 2)
        type = Request::Type::WRITE;
    else if (queue_type == 3)
        return get_size(1, address);

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    os_transparent_management.physical_to_hardware_address(address);
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, type);
        return memory.get_queue_size(request);
    }
    else if (address < memory.max_address + memory2.max_address)
    {
        // the memory itself doesn't know other memories' space, so we manage the overall mapping.
        Request request(address - memory.max_address, type);
        return memory2.get_queue_size(request);
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::return_data(Request& request)
{
    // recover the hardware address to physical address.
    switch (request.memory_id)
    {
    case MEMORY_NUMBER_ONE:
        // nothing to do
        break;
    case MEMORY_NUMBER_TWO:
        request.addr += memory.max_address;
        break;
    default:
    {
        std::cout << __func__ << ": return_data error." << std::endl;
        assert(0);
    }
    break;
    }

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE) && (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    bool finish_return_data = false;

    if (uint64_t(request.addr) < memory.max_address)
    {
        // this could be an uncomplete write request
        bool finish = os_transparent_management.finish_fm_access_in_incomplete_write_request_queue(request.packet.h_address);
        if (finish)
        {
            finish_return_data = true;
            return;
        }
    }

    if ((uint64_t(request.addr) < memory.max_address) && (memory.max_address <= request.packet.h_address))
    {
        // this could be an uncomplete read request
        bool finish = os_transparent_management.finish_fm_access_in_incomplete_read_request_queue(request.packet.h_address);

        if (finish)
        {
            finish_return_data = true;
            return;
        }
    }

    if (finish_return_data == false)
    {
        // this is a complete read request
        for (auto ret : request.packet.to_return)
            ret->return_data(&(request.packet));
    }

#else
    for (auto ret : request.packet.to_return)
        ret->return_data(&(request.packet));
#endif // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT && COLOCATED_LINE_LOCATION_TABLE
};

#if (MEMORY_USE_SWAPPING_UNIT == ENABLE)
// functions for swapping
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

// input address should be hardware address and at byte granularity
template<class T, class T2>
bool MEMORY_CONTROLLER<T, T2>::start_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
{
    assert(size <= SWAPPING_BUFFER_ENTRY_NUMBER);

    if (states == SwappingState::Idle)
    {
        states              = SwappingState::Swapping;      // start swapping.
        base_address[0]     = address_1 >> LOG2_BLOCK_SIZE; // the single swapping is conducted at cache line granularity.
        base_address[1]     = address_2 >> LOG2_BLOCK_SIZE;
        active_entry_number = size;
    }
    else
    {
        return false; // this swapping unit is busy, it cannot issue new swapping request.
    }
    return true; // new swapping is issued.
}

// input address should be hardware address and at byte granularity
template<class T, class T2>
bool MEMORY_CONTROLLER<T, T2>::update_swapping_segments(uint64_t address_1, uint64_t address_2, uint8_t size)
{
    assert(size <= SWAPPING_BUFFER_ENTRY_NUMBER);

    uint64_t input_base_address[SWAPPING_SEGMENT_NUMBER];

    input_base_address[0] = address_1 >> LOG2_BLOCK_SIZE;
    input_base_address[1] = address_2 >> LOG2_BLOCK_SIZE;

    if ((input_base_address[0] == base_address[0]) && (input_base_address[1] == base_address[1]))
    {
        // they are same swapping segments
        if (size > active_entry_number)
        {
            // there have new data to swap
            active_entry_number = size;
            if (states == SwappingState::Idle)
            {
                states = SwappingState::Swapping; // start swapping.
            }

            return true; // update swapping segments
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
        for (auto i = 0; i < active_entry_number; i++) // go through the active buffer
        {
            if (buffer[i].finish == false)
            {
                for (auto j = 0; j < SWAPPING_SEGMENT_NUMBER; j++)
                {
                    if (buffer[i].read_issue[j] == false)
                    {
                        bool stall       = true;
                        uint64_t address = (base_address[j] + i) << LOG2_BLOCK_SIZE;

                        // assign the request to the right memory.
                        if (address < memory.max_address)
                        {
                            Request request(address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_swapping_data, this, placeholders::_1), coreid, memory_id);
                            stall = ! memory.send(request);
                        }
                        else if (address < memory.max_address + memory2.max_address)
                        {
                            // the memory itself doesn't know other memories' space, so we manage the overall mapping.
                            Request request(address - memory.max_address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T, T2>::return_swapping_data, this, placeholders::_1), coreid, memory2_id);
                            stall = ! memory2.send(request);
                        }
                        else
                        {
                            printf("%s: Error!\n", __FUNCTION__);
                        }

#if (PRINT_MEMORY_TRACE == ENABLE)
                        // output memory trace.
                        output_memorytrace.output_memory_trace_hexadecimal(address, 'R');
#endif // PRINT_MEMORY_TRACE

                        if (stall == true)
                        {
                            // queue is full
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
        for (auto i = 0; i < active_entry_number; i++) // go through the active buffer
        {
            if (buffer[i].finish == false)
            {
                if ((buffer[i].read[0] == true) && (buffer[i].read[1] == true)) // read requests are finished
                {
                    for (auto j = 0; j < SWAPPING_SEGMENT_NUMBER; j++)
                    {
                        if ((buffer[i].write[j] == false) || (buffer[i].dirty[j] == true))
                        {
                            bool stall       = true;
                            uint64_t address = (base_address[j] + i) << LOG2_BLOCK_SIZE;

                            // assign the request to the right memory.
                            if (address < memory.max_address)
                            {
                                Request request(address, Request::Type::WRITE, NULL, coreid, memory_id);
                                // get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory.send(request);
                            }
                            else if (address < memory.max_address + memory2.max_address)
                            {
                                // the memory itself doesn't know other memories' space, so we manage the overall mapping.
                                Request request(address - memory.max_address, Request::Type::WRITE, NULL, coreid, memory2_id);
                                // get data from buffer
                                request.data = buffer[i].data[j];
                                stall        = ! memory2.send(request);
                            }
                            else
                            {
                                printf("%s: Error!\n", __FUNCTION__);
                            }

#if (PRINT_MEMORY_TRACE == ENABLE)
                            // output memory trace.
                            output_memorytrace.output_memory_trace_hexadecimal(address, 'W');
#endif // PRINT_MEMORY_TRACE

                            if (stall == true)
                            {
                                // queue is full
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

                // finish swapping
                if ((buffer[i].write[0] == true) && (buffer[i].write[1] == true))
                {
                    buffer[i].finish = true;
                    finish_number++;
                }
            }
        }

        // check finish_number
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

// this function is used by memories, like Ramulator.
template<class T, class T2>
void MEMORY_CONTROLLER<T, T2>::return_swapping_data(Request& request)
{
    // sanity check
    // assert(states == SwappingState::Swapping);
    if (states == SwappingState::Idle)
    {
        std::cout << __func__ << ": data is returned while swapping is already finished." << std::endl;
        return;
    }

    // recover the hardware address to physical address.
    switch (request.memory_id)
    {
    case MEMORY_NUMBER_ONE:
        // nothing to do
        break;
    case MEMORY_NUMBER_TWO:
        request.addr += memory.max_address;
        break;
    default:
    {
        std::cout << __func__ << ": swapping error." << std::endl;
        assert(0);
    }
    break;
    }

    uint64_t address = request.addr >> LOG2_BLOCK_SIZE;
    uint8_t segment_index;
    uint8_t entry_index;
    // calculate entry index in the fashion of little-endian.
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
        assert(0);
    }

    // Read data
    if ((buffer[entry_index].finish == false) && (buffer[entry_index].write[segment_index] == false) && (buffer[entry_index].dirty[segment_index] == false))
    {
        buffer[entry_index].data[segment_index] = request.data;
        buffer[entry_index].read[segment_index] = true;
    }
};

template<class T, class T2>
uint8_t MEMORY_CONTROLLER<T, T2>::check_request(PACKET& packet, uint8_t type)
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
    // calculate entry index in the fashion of little-endian.
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
        return 1; // this address is not under swapping.
    }

    if (type == 1) // for read request
    {
        if ((buffer[entry_index].finish == true) || (buffer[entry_index].read[segment_index] == true) || (buffer[entry_index].write[segment_index] == true) || (buffer[entry_index].dirty[segment_index] == true))
        {
            uint64_t& read_data = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // note the PACKET only has 64 bit data.
            packet.data         = read_data;

            return 2; // though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else if (type == 2) // for write request
    {
        if ((buffer[entry_index].read[0] == true) && (buffer[entry_index].read[1] == true))
        {
            uint64_t& write_data                     = *((uint64_t*) (&buffer[entry_index].data[segment_index])); // note the PACKET only has 64 bit data.
            write_data                               = packet.data;
            buffer[entry_index].dirty[segment_index] = true;

            if (buffer[entry_index].finish == true)
            {
                --finish_number;
            }
            buffer[entry_index].finish = false;

            return 2; // though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else
    {
        std::cout << __func__ << ": type input error." << std::endl;
        assert(0);
    }

    return 0; // this address is under swapping.
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
    // calculate entry index in the fashion of little-endian.
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
        return 1; // this address is not under swapping.
    }

    if (type == 1) // for read request
    {
        if ((buffer[entry_index].finish == true) || (buffer[entry_index].read[segment_index] == true) || (buffer[entry_index].write[segment_index] == true) || (buffer[entry_index].dirty[segment_index] == true))
        {
            return 2; // though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else if (type == 2) // for write request
    {
        if ((buffer[entry_index].read[0] == true) && (buffer[entry_index].read[1] == true))
        {
            return 2; // though this address is under swapping, we can service its request because the data is in the swapping buffer.
        }
    }
    else
    {
        std::cout << __func__ << ": type input error." << std::endl;
        assert(0);
    }

    return 0; // this address is under swapping.
};

#endif // MEMORY_USE_SWAPPING_UNIT

#else
template<typename T>
class MEMORY_CONTROLLER : public champsim::operable, public MemoryRequestConsumer
{
public:
    /* General part */
    double clock_scale = MEMORY_CONTROLLER_CLOCK_SCALE;

    // Note here they are the references to escape memory deallocation here.
    ramulator::Memory<T, Controller>& memory;

    const uint8_t memory_id = 0;

    uint64_t read_request_in_memory;
    uint64_t write_request_in_memory;

    /* Member functions */
    MEMORY_CONTROLLER(double freq_scale, double clock_scale, Memory<T, Controller>& memory);
    ~MEMORY_CONTROLLER();

    void operate() override;
    void print_deadlock() override;

    int add_rq(PACKET* packet) override;
    int add_wq(PACKET* packet) override;
    int add_pq(PACKET* packet) override;

    /** @brief
     *  Get the number of valid(active) member in the write/read queue.
     *  The address is physical address.
     */
    uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;

    /** @brief
     *  Get the capacity of the write/read queue.
     *  The address is physical address.
     */
    uint32_t get_size(uint8_t queue_type, uint64_t address) override;

    void return_data(Request& request);
};

template<class T>
MEMORY_CONTROLLER<T>::MEMORY_CONTROLLER(double freq_scale, double clock_scale, Memory<T, Controller>& memory)
: champsim::operable(freq_scale), MemoryRequestConsumer(std::numeric_limits<unsigned>::max()),
  clock_scale(clock_scale), memory(memory)
{
    read_request_in_memory  = 0;
    write_request_in_memory = 0;
};

template<class T>
MEMORY_CONTROLLER<T>::~MEMORY_CONTROLLER()
{
    // print some information to output_statistics
    output_statistics.read_request_in_memory  = read_request_in_memory;
    output_statistics.write_request_in_memory = write_request_in_memory;
};

template<class T>
void MEMORY_CONTROLLER<T>::operate()
{
    /* Operate memories below */
    static double leap_operation = 0;
    // skip periodically
    if (leap_operation >= 1)
    {
        leap_operation -= 1;
    }
    else
    {
        memory.tick();
        leap_operation += clock_scale;
    }

    Stats::curTick++; // processor clock, global, for Statistics
};

template<class T>
void MEMORY_CONTROLLER<T>::print_deadlock()
{
    printf("MEMORY_CONTROLLER print_deadlock().\n");
};

template<class T>
int MEMORY_CONTROLLER<T>::add_rq(PACKET* packet)
{
    const static uint8_t type = 1; // it means the input request is read request.

    if (all_warmup_complete < NUM_CPUS)
    {
        for (auto ret : packet->to_return)
            ret->return_data(packet);

        return int(ReturnValue::Forward); // Fast-forward
    }

    /* Send memory request below */
    bool stall       = true;

    uint64_t address = packet->address;

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, Request::Type::READ, std::bind(&MEMORY_CONTROLLER<T>::return_data, this, placeholders::_1), *packet, packet->cpu, memory_id);
        stall = ! memory.send(request);

        if (stall == false)
        {
            read_request_in_memory++;
        }
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // output memory trace.
    output_memorytrace.output_memory_trace_hexadecimal(packet->address, 'R');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return get_occupancy(type, packet->address);
    }
};

template<class T>
int MEMORY_CONTROLLER<T>::add_wq(PACKET* packet)
{
    const static uint8_t type = 2; // it means the input request is write request.

    if (all_warmup_complete < NUM_CPUS)
        return int(ReturnValue::Forward); // Fast-forward

    bool stall       = true;

    uint64_t address = packet->address;

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, Request::Type::WRITE, std::bind(&MEMORY_CONTROLLER<T>::return_data, this, placeholders::_1), *packet, packet->cpu, memory_id);
        stall = ! memory.send(request);

        if (stall == false)
        {
            write_request_in_memory++;
        }
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

#if (PRINT_MEMORY_TRACE == ENABLE)
    // output memory trace.
    output_memorytrace.output_memory_trace_hexadecimal(packet->address, 'W');
#endif // PRINT_MEMORY_TRACE

    if (stall == true)
    {
        return int(ReturnValue::Full); // queue is full, note Ramulator doesn't merge requests.
    }
    else
    {
        return get_occupancy(type, packet->address);
    }
};

template<class T>
int MEMORY_CONTROLLER<T>::add_pq(PACKET* packet)
{
    return add_rq(packet);
};

template<class T>
uint32_t MEMORY_CONTROLLER<T>::get_occupancy(uint8_t queue_type, uint64_t address)
{
    Request::Type type = Request::Type::READ;

    if (queue_type == 1)
        type = Request::Type::READ;
    else if (queue_type == 2)
        type = Request::Type::WRITE;
    else if (queue_type == 3)
        return get_occupancy(1, address);

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, type);
        return memory.get_queue_occupancy(request);
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T>
uint32_t MEMORY_CONTROLLER<T>::get_size(uint8_t queue_type, uint64_t address)
{
    Request::Type type = Request::Type::READ;

    if (queue_type == 1)
        type = Request::Type::READ;
    else if (queue_type == 2)
        type = Request::Type::WRITE;
    else if (queue_type == 3)
        return get_size(1, address);

    // assign the request to the right memory.
    if (address < memory.max_address)
    {
        Request request(address, type);
        return memory.get_queue_size(request);
    }
    else
    {
        printf("%s: Error!\n", __FUNCTION__);
    }

    return 0;
};

template<class T>
void MEMORY_CONTROLLER<T>::return_data(Request& request)
{
    for (auto ret : request.packet.to_return)
        ret->return_data(&(request.packet));
};
#endif // MEMORY_USE_HYBRID
#else
class MEMORY_CONTROLLER : public champsim::operable, public MemoryRequestConsumer
{
public:
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
    const static uint64_t tRP                        = detail::ceil(1.0 * tRP_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t tRCD                       = detail::ceil(1.0 * tRCD_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t tCAS                       = detail::ceil(1.0 * tCAS_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t DRAM_DBUS_TURN_AROUND_TIME = detail::ceil(1.0 * DBUS_TURN_AROUND_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t DRAM_DBUS_RETURN_TIME      = detail::ceil(1.0 * BLOCK_SIZE / DRAM_CHANNEL_WIDTH);

    MEMORY_STATISTICS statistics;

    std::array<DDR_CHANNEL, DDR_CHANNELS> ddr_channels;
    std::array<HBM_CHANNEL, HBM_CHANNELS> hbm_channels;

    MEMORY_CONTROLLER(double freq_scale): champsim::operable(freq_scale), MemoryRequestConsumer(std::numeric_limits<unsigned>::max()) {}

    void operate() override;

    int add_rq(PACKET* packet) override;
    int add_wq(PACKET* packet) override;
    int add_pq(PACKET* packet) override;

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;
    uint32_t get_size(uint8_t queue_type, uint64_t address) override;

    uint32_t memory_get_channel(uint64_t address);
    uint32_t memory_get_bank(uint64_t address);
    uint32_t memory_get_column(uint64_t address);
    uint32_t memory_get_row(uint64_t address);

    // HBM doesn't have rank.
    uint32_t hbm_get_channel(uint64_t address);
    uint32_t hbm_get_bank(uint64_t address);
    uint32_t hbm_get_column(uint64_t address);
    uint32_t hbm_get_row(uint64_t address);

    uint32_t ddr_get_channel(uint64_t address);
    uint32_t ddr_get_rank(uint64_t address);
    uint32_t ddr_get_bank(uint64_t address);
    uint32_t ddr_get_column(uint64_t address);
    uint32_t ddr_get_row(uint64_t address);

    MemoryType get_memory_type(uint64_t address);

    float get_average_memory_access_time();

private:
    void operate_hbm(HBM_CHANNEL& channel);
    void operate_ddr(DDR_CHANNEL& channel);
};
#endif // RAMULATOR

#else
struct BANK_REQUEST
{
    bool valid = false, row_buffer_hit = false;

    std::size_t open_row = std::numeric_limits<uint32_t>::max();

    uint64_t event_cycle = 0;

    std::vector<PACKET>::iterator pkt;
};

struct DRAM_CHANNEL
{
    std::vector<PACKET> WQ {DRAM_WQ_SIZE};
    std::vector<PACKET> RQ {DRAM_RQ_SIZE};

    std::array<BANK_REQUEST, DRAM_RANKS* DRAM_BANKS> bank_request             = {};
    std::array<BANK_REQUEST, DRAM_RANKS* DRAM_BANKS>::iterator active_request = std::end(bank_request);

    uint64_t dbus_cycle_available = 0, dbus_cycle_congested = 0, dbus_count_congested = 0;

    bool write_mode            = false;

    unsigned WQ_ROW_BUFFER_HIT = 0, WQ_ROW_BUFFER_MISS = 0, RQ_ROW_BUFFER_HIT = 0, RQ_ROW_BUFFER_MISS = 0, WQ_FULL = 0;
};

class MEMORY_CONTROLLER : public champsim::operable, public MemoryRequestConsumer
{
public:
    // DRAM_IO_FREQ defined in champsim_constants.h
    const static uint64_t tRP                        = detail::ceil(1.0 * tRP_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t tRCD                       = detail::ceil(1.0 * tRCD_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t tCAS                       = detail::ceil(1.0 * tCAS_DRAM_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t DRAM_DBUS_TURN_AROUND_TIME = detail::ceil(1.0 * DBUS_TURN_AROUND_NANOSECONDS * DRAM_IO_FREQ / 1000);
    const static uint64_t DRAM_DBUS_RETURN_TIME      = detail::ceil(1.0 * BLOCK_SIZE / DRAM_CHANNEL_WIDTH);

    std::array<DRAM_CHANNEL, DRAM_CHANNELS> channels;

    MEMORY_CONTROLLER(double freq_scale): champsim::operable(freq_scale), MemoryRequestConsumer(std::numeric_limits<unsigned>::max()) {}

    int add_rq(PACKET* packet) override;
    int add_wq(PACKET* packet) override;
    int add_pq(PACKET* packet) override;

    void operate() override;

    uint32_t get_occupancy(uint8_t queue_type, uint64_t address) override;
    uint32_t get_size(uint8_t queue_type, uint64_t address) override;

    uint32_t dram_get_channel(uint64_t address);
    uint32_t dram_get_rank(uint64_t address);
    uint32_t dram_get_bank(uint64_t address);
    uint32_t dram_get_row(uint64_t address);
    uint32_t dram_get_column(uint64_t address);
};
#endif // USER_CODES

#endif

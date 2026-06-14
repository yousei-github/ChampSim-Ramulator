// Use Ramulator 2.0 to simulate the memory

#ifndef RAMULATOR2_DRAM_CONTROLLER_H
#define RAMULATOR2_DRAM_CONTROLLER_H

/* Header */

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE) && (RAMULATOR2 == ENABLE)

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "ChampSim/address.h"
#include "ChampSim/channel.h"
#include "ChampSim/chrono.h"
#include "ChampSim/dram_stats.h"
#include "ChampSim/operable.h"
#include "Ramulator2/base/request.h"

#if (MEMORY_USE_HYBRID == ENABLE)
#include <array>

#include "os_transparent_management.h"
#endif /* MEMORY_USE_HYBRID */

/* Macro */

/* Type */

namespace Ramulator
{
class IFrontEnd;
class IMemorySystem;
struct Request;
} // namespace Ramulator

/* Prototype */

#if (MEMORY_USE_HYBRID == ENABLE)
// Enable hybrid memory system

class MEMORY_CONTROLLER : public champsim::operable
{
    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    std::string yaml_path, yaml_path2;
    Ramulator::IFrontEnd* frontend           = nullptr; // Fast memory frontend
    Ramulator::IMemorySystem* memory_system  = nullptr; // Fast memory system
    Ramulator::IFrontEnd* frontend2          = nullptr; // Slow memory frontend
    Ramulator::IMemorySystem* memory_system2 = nullptr; // Slow memory system
    double io_freq_scale                     = {};
    double io_freq_scale2                    = {};

    /** @todo add similar code in include/ChampSim/ramulator_dram_controller.h */

    // Cached capacities [Byte]. memory.max_address in the Ramulator 1.0 controller.
    uint64_t max_address  = 0; // Fast memory capacity
    uint64_t max_address2 = 0; // Slow memory capacity

    /** @brief Memory request type */
    enum class RequestType : int
    {
        Read = 0,
        Write,
        Max
    };

    void initiate_requests();
    bool add_rq(request_type& packet, champsim::channel* ul);
    bool add_wq(request_type& packet);

public:
    const uint8_t memory_id  = MEMORY_NUMBER_ONE;
    const uint8_t memory2_id = MEMORY_NUMBER_TWO;

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
    /** @todo add similar code in include/ChampSim/ramulator_dram_controller.h */

    // Built in the constructor body once both memory systems exist (capacities
    // are unknown until the YAML configs are parsed), so a pointer rather than a
    // reference like the Ramulator 1.0 controller.
    OS_TRANSPARENT_MANAGEMENT* os_transparent_management = nullptr;
#endif /* MEMORY_USE_OS_TRANSPARENT_MANAGEMENT */

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
#endif /* TEST_SWAPPING_UNIT */

#endif /* MEMORY_USE_SWAPPING_UNIT */

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
    uint64_t load_request_in_memory, load_request_in_memory2;
    uint64_t store_request_in_memory, store_request_in_memory2;
#endif /* TRACKING_LOAD_STORE_STATISTICS */

    uint64_t read_request_in_memory, read_request_in_memory2;
    uint64_t write_request_in_memory, write_request_in_memory2;

    MEMORY_CONTROLLER(champsim::chrono::picoseconds mc_period, std::vector<channel_type*>&& ul, std::string configs, std::string configs2);
    ~MEMORY_CONTROLLER();

    void initialize() final;
    long operate() final;
    void begin_phase() final;
    void end_phase(unsigned cpu) final;
    void print_deadlock() final;

    /**
     * @brief
     * Get the size of the physical space of the memory system
     *
     * @return Size of the physical space [Byte]
     */
    [[nodiscard]] champsim::data::bytes size() const;

    /**
     * @brief Get the number of valid members in the related write/read queue
     * @param[in] queue_type The type of queue
     * @param[in] address    The address related to queue
     * @return    Number of valid members
     *
     * @note The address is physical address
     */
    size_t get_occupancy(RequestType queue_type, uint64_t address);

    /**
     * @brief Get the capacity of the related write/read queue
     * @param[in] queue_type The type of queue
     * @param[in] address    The address related to queue
     * @return    Capacity
     *
     * @note The address is physical address
     */
    size_t get_queue_size(RequestType queue_type, uint64_t address);

    /**
     * @brief Return responses (like data) to LLC when the request is completed by memory system
     * @note Write requests don't return responses to LLC
     */
    void return_data(Ramulator::Request& request);

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
    void return_swapping_data(Ramulator::Request& request);

    // This function is used by memory controller in add_rq() and add_wq().
    uint8_t check_request(request_type& packet, OS_TRANSPARENT_MANAGEMENT::MemoryRequestType type); // Packet needs to prepare its hardware address.
    uint8_t check_address(uint64_t address, uint8_t type);                      // The address is physical address.

#endif /* MEMORY_USE_SWAPPING_UNIT */
};

#else
// Disable hybrid memory system

class MEMORY_CONTROLLER : public champsim::operable
{
    using channel_type  = champsim::channel;
    using request_type  = typename channel_type::request_type;
    using response_type = typename channel_type::response_type;
    std::vector<channel_type*> queues;

    std::string yaml_path;
    Ramulator::IFrontEnd* frontend          = nullptr;
    Ramulator::IMemorySystem* memory_system = nullptr;
    double io_freq_scale                    = {};

    /** @brief Memory request type */
    enum class RequestType : int
    {
        Read = 0,
        Write,
        Max
    };

    void initiate_requests();
    bool add_rq(const request_type& packet, champsim::channel* ul);
    bool add_wq(const request_type& packet);

public:
    const uint8_t memory_id = 0;

    uint64_t read_request_in_memory;
    uint64_t write_request_in_memory;

    MEMORY_CONTROLLER(champsim::chrono::picoseconds mc_period, std::vector<channel_type*>&& ul, std::string configs);
    ~MEMORY_CONTROLLER();

    void initialize() final;
    long operate() final;
    void begin_phase() final;
    void end_phase(unsigned cpu) final;
    void print_deadlock() final;

    /**
     * @brief
     * Get the size of the physical space of the memory system
     *
     * @return Size of the physical space [Byte]
     */
    [[nodiscard]] champsim::data::bytes size() const;

    /**
     * @brief Get the number of valid members in the related write/read queue
     * @param[in] queue_type The type of queue
     * @param[in] address    The address related to queue
     * @return    Number of valid members
     *
     * @note The address is physical address
     */
    size_t get_occupancy(RequestType queue_type, uint64_t address);

    /**
     * @brief Get the capacity of the related write/read queue
     * @param[in] queue_type The type of queue
     * @param[in] address    The address related to queue
     * @return    Capacity
     *
     * @note The address is physical address
     */
    size_t get_queue_size(RequestType queue_type, uint64_t address);

    /**
     * @brief Return responses (like data) to LLC when the request is completed by memory system
     * @note Write requests don't return responses to LLC
     */
    void return_data(Ramulator::Request& request);
};

#endif /* MEMORY_USE_HYBRID */

#endif /* USER_CODES && RAMULATOR2 */

#endif /* RAMULATOR2_DRAM_CONTROLLER_H */

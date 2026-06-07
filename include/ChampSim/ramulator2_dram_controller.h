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

// Use Ramulator 2.0 to simulate the memory

/* Macro */

/* Type */

namespace Ramulator
{
class IFrontEnd;
class IMemorySystem;
struct Request;
} // namespace Ramulator

/* Prototype */

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

#endif /* USER_CODES && RAMULATOR2 */

#endif /* RAMULATOR2_DRAM_CONTROLLER_H */

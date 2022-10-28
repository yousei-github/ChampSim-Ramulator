#ifndef OS_TRANSPARENT_MANAGEMENT_H
#define OS_TRANSPARENT_MANAGEMENT_H
#include <map>
#include <vector>
#include <iostream>
#include <cassert>
#include <deque>

#include "champsim_constants.h"
#include "memory_class.h"
#include "util.h"
#include "ProjectConfiguration.h" // user file



/** @note Abbreviation:
 *  FM -> Fast memory (e.g., HBM, DDR4)
 *  SM -> Slow memory (e.g., DDR4, PCM)
 *
*/

#define COUNTER_WIDTH                   uint8_t
#define COUNTER_MAX_VALUE               (UINT8_MAX)
#define COUNTER_DEFAULT_VALUE           (0)

#define HOTNESS_WIDTH                   bool
#define HOTNESS_DEFAULT_VALUE           (false)

#define REMAPPING_LOCATION_WIDTH            uint8_t
#define REMAPPING_LOCATION_WIDTH_BITS       (3)
#define LOCATION_TABLE_ENTRY_WIDTH          uint16_t

// 0x0538 for the congruence group with 5 members (lines) at most, (000_001_010_011_100_0 = 0x0538)
// [15:13] bit for member 0, [12:10] bit for member 1, [9:7] bit for member 2, [6:4] bit for member 3, [3:1] bit for member 4.
#define LOCATION_TABLE_ENTRY_DEFAULT_VALUE  (0x0538)
#define LOCATION_TABLE_ENTRY_MSB            (UINT16_WIDTH - 1)  // MSB -> most significant bit

#define REMAPPING_REQUEST_QUEUE_LENGTH      (64)  // 1024/4096
#define QUEUE_BUSY_DEGREE_THRESHOLD         (0.8f)

class OS_TRANSPARENT_MANAGEMENT
{
public:
    COUNTER_WIDTH hotness_threshold = 0;
    uint64_t total_capacity;        // uint is byte
    uint64_t fast_memory_capacity;  // uint is byte
    uint64_t fast_memory_capacity_at_data_block_granularity;
    uint8_t  fast_memory_offset_bit;    // address format in the byte granularity
    uint8_t  congruence_group_msb;      // most significant bit of congruence group, and its address format is in the byte granularity
    std::vector<COUNTER_WIDTH>& counter_table;      // A counter for every data block
    std::vector<HOTNESS_WIDTH>& hotness_table;      // A hotness bit for every data block, true -> data block is hot, false -> data block is cold.

    // scoped enumerations
    enum class RemappingLocation : REMAPPING_LOCATION_WIDTH
    {
        Zero = 0, One, Two, Three, Four,
        Max
    };

    /* Remapping table */
    std::vector<LOCATION_TABLE_ENTRY_WIDTH>& line_location_table;    // paper CAMEO: SRAM-Based LLT / Embed LLT in Stacked DRAM

    /* Remapping request */
    struct RemappingRequest
    {
        uint64_t address_in_fm, address_in_sm;  // hardware address
        REMAPPING_LOCATION_WIDTH fm_location, sm_location;
    };
    std::deque<RemappingRequest> remapping_request_queue;
    uint64_t remapping_request_queue_congestion;

    OS_TRANSPARENT_MANAGEMENT(COUNTER_WIDTH threshold, uint64_t max_address, uint64_t fast_memory_max_address)
        : hotness_threshold(threshold), total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
        fast_memory_capacity_at_data_block_granularity(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS),
        fast_memory_offset_bit(lg2(fast_memory_max_address)),   // note here only support integers of 2's power.
        congruence_group_msb(REMAPPING_LOCATION_WIDTH_BITS + fast_memory_offset_bit - 1),
        counter_table(*(new std::vector<COUNTER_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, COUNTER_DEFAULT_VALUE))),
        hotness_table(*(new std::vector<HOTNESS_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, HOTNESS_DEFAULT_VALUE))),
        line_location_table(*(new std::vector<LOCATION_TABLE_ENTRY_WIDTH>(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS, LOCATION_TABLE_ENTRY_DEFAULT_VALUE)))
    {
        remapping_request_queue_congestion = 0;

        uint64_t expected_number_in_congruence_group = total_capacity / fast_memory_capacity;
        if (expected_number_in_congruence_group > REMAPPING_LOCATION_WIDTH(RemappingLocation::Max))
        {
            std::cout << __func__ << ": congruence group error." << std::endl;
        }
        else
        {
            printf("Number in Congruence group: %ld.\n", expected_number_in_congruence_group);
        }

    };

    ~OS_TRANSPARENT_MANAGEMENT()
    {
        outputchampsimstatistics.remapping_request_queue_congestion = remapping_request_queue_congestion;
        delete& counter_table;
        delete& hotness_table;
        delete& line_location_table;
    };

    // address is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree)
    {
        if (address >= total_capacity)
        {
            std::cout << __func__ << ": address input error." << std::endl;
            return false;
        }

        uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
        uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in line location table
        REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the location in the entry of the line location table

        if (location >= REMAPPING_LOCATION_WIDTH(RemappingLocation::Max))
        {
            std::cout << __func__ << ": address input error (location)." << std::endl;
            abort();
        }

        uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
        uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

        REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

        if (type == 1)  // for read request
        {
            if (counter_table.at(data_block_address) < COUNTER_MAX_VALUE)
            {
                counter_table[data_block_address]++;    // increment its counter
            }

            if (counter_table.at(data_block_address) >= hotness_threshold)
            {
                hotness_table.at(data_block_address) = true;    // mark hot data block
            }

        }
        else if (type == 2) // for write request
        {
            if (counter_table.at(data_block_address) < COUNTER_MAX_VALUE)
            {
                counter_table[data_block_address]++;    // increment its counter
            }

            if (counter_table.at(data_block_address) >= hotness_threshold)
            {
                hotness_table.at(data_block_address) = true;    // mark hot data block
            }
        }
        else
        {
            std::cout << __func__ << ": type input error." << std::endl;
            assert(0);
        }

        // add new remapping requests to queue
        if ((hotness_table.at(data_block_address) == true) && (remapping_location != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
        {
            RemappingRequest remapping_request;
            REMAPPING_LOCATION_WIDTH fm_location = REMAPPING_LOCATION_WIDTH(RemappingLocation::Max);

            uint8_t fm_msb_in_location_table_entry;
            uint8_t fm_lsb_in_location_table_entry;
            REMAPPING_LOCATION_WIDTH fm_remapping_location;

            // find the fm_location in the entry of line_location_table (where RemappingLocation::Zero is in the entry of line_location_table)
            for (REMAPPING_LOCATION_WIDTH i = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero); i < REMAPPING_LOCATION_WIDTH(RemappingLocation::Max); i++)
            {
                fm_msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * i;
                fm_lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * i - 1);

                fm_remapping_location = get_bits(line_location_table.at(line_location_table_index), fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);

                if (fm_remapping_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
                {
                    // found the fm_location in the entry of line_location_table
                    fm_location = i;
                    break;
                }
            }

            if (fm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Max))
            {
                std::cout << __func__ << ": find the fm_location error." << std::endl;
                abort();
            }

            if (fm_remapping_location == remapping_location)    // check
            {
                std::cout << __func__ << ": add new remapping request error 1." << std::endl;
                printf("line_location_table.at(%ld): %d.\n", line_location_table_index, line_location_table.at(line_location_table_index));
                printf("remapping_location: %d, fm_remapping_location: %d.\n", remapping_location, fm_remapping_location);
                printf("fm_msb_in_location_table_entry: %d, fm_lsb_in_location_table_entry: %d.\n", fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);
                printf("msb_in_location_table_entry: %d, lsb_in_location_table_entry: %d.\n", msb_in_location_table_entry, lsb_in_location_table_entry);
                abort();
            }

            remapping_request.address_in_fm = replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(fm_remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit);
            remapping_request.address_in_sm = replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit);

            remapping_request.fm_location = fm_location;
            remapping_request.sm_location = location;

            // check duplicated remapping request in remapping_request_queue
            // if duplicated remapping requests exist, we won't add this new remapping request into the remapping_request_queue.
            bool duplicated_remapping_request = false;
            for (uint64_t i = 0; i < remapping_request_queue.size(); i++)
            {
                if ((remapping_request_queue[i].address_in_fm == remapping_request.address_in_fm) || (remapping_request_queue[i].address_in_sm == remapping_request.address_in_fm))
                {
                    duplicated_remapping_request = true;    // find a duplicated remapping request
                    break;
                }
                if ((remapping_request_queue[i].address_in_fm == remapping_request.address_in_sm) || (remapping_request_queue[i].address_in_sm == remapping_request.address_in_sm))
                {
                    duplicated_remapping_request = true;    // find a duplicated remapping request
                    break;
                }
            }

            if ((duplicated_remapping_request == false) && (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD))
            {
                if (remapping_request_queue.size() < REMAPPING_REQUEST_QUEUE_LENGTH)
                {
                    if (remapping_request.address_in_fm == remapping_request.address_in_sm)    // check
                    {
                        std::cout << __func__ << ": add new remapping request error 2." << std::endl;
                        abort();
                    }

                    // enqueue a remapping request
                    remapping_request_queue.push_back(remapping_request);
                }
                else
                {
                    //std::cout << __func__ << ": remapping_request_queue is full." << std::endl;
                    remapping_request_queue_congestion++;
                }
            }
        }

        return true;
    };

    void physical_to_hardware_address(PACKET& packet)
    {
        uint64_t data_block_address = packet.address >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
        REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);

        uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
        uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

        REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

        packet.h_address = replace_bits(replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit), packet.address, DATA_MANAGEMENT_OFFSET_BITS - 1);
    };

    void physical_to_hardware_address(uint64_t& address)
    {
        uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
        REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);

        uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
        uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

        REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

        address = replace_bits(replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit), address, DATA_MANAGEMENT_OFFSET_BITS - 1);
    };

    bool issue_remapping_request(RemappingRequest& remapping_request)
    {
        if (remapping_request_queue.empty() == false)
        {
            remapping_request = remapping_request_queue.front();
            return true;
        }

        return false;
    };

    bool finish_remapping_request()
    {
        if (remapping_request_queue.empty() == false)
        {
            RemappingRequest remapping_request = remapping_request_queue.front();
            remapping_request_queue.pop_front();

            uint64_t data_block_address = remapping_request.address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
            //data_block_address = remapping_request.address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;
            uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;

            uint8_t fm_msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * remapping_request.fm_location;
            uint8_t fm_lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * remapping_request.fm_location - 1);
            uint8_t sm_msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * remapping_request.sm_location;
            uint8_t sm_lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * remapping_request.sm_location - 1);

            REMAPPING_LOCATION_WIDTH fm_remapping_location = get_bits(line_location_table.at(line_location_table_index), fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);
            REMAPPING_LOCATION_WIDTH sm_remapping_location = get_bits(line_location_table.at(line_location_table_index), sm_msb_in_location_table_entry, sm_lsb_in_location_table_entry);

            line_location_table.at(line_location_table_index) = replace_bits(line_location_table.at(line_location_table_index), uint64_t(fm_remapping_location) << sm_lsb_in_location_table_entry, sm_msb_in_location_table_entry, sm_lsb_in_location_table_entry);
            line_location_table.at(line_location_table_index) = replace_bits(line_location_table.at(line_location_table_index), uint64_t(sm_remapping_location) << fm_lsb_in_location_table_entry, fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);

            if (fm_remapping_location == sm_remapping_location)
            {
                std::cout << __func__ << ": read remapping location error." << std::endl;
                abort();
            }
        }
        else
        {
            std::cout << __func__ << ": remapping error." << std::endl;
            assert(0);
            return false;   // error
        }

        return true;
    };
};

#endif  // OS_TRANSPARENT_MANAGEMENT_H

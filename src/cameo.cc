#include "os_transparent_management.h"

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_LINE_LOCATION_TABLE == ENABLE) || (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
OS_TRANSPARENT_MANAGEMENT::OS_TRANSPARENT_MANAGEMENT(COUNTER_WIDTH threshold, uint64_t max_address, uint64_t fast_memory_max_address)
    : hotness_threshold(threshold), total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
    total_capacity_at_data_block_granularity(max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_capacity_at_data_block_granularity(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_offset_bit(lg2(fast_memory_max_address)),   // note here only support integers of 2's power.
    counter_table(*(new std::vector<COUNTER_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, COUNTER_DEFAULT_VALUE))),
    hotness_table(*(new std::vector<HOTNESS_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, HOTNESS_DEFAULT_VALUE))),
    congruence_group_msb(REMAPPING_LOCATION_WIDTH_BITS + fast_memory_offset_bit - 1),
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

OS_TRANSPARENT_MANAGEMENT::~OS_TRANSPARENT_MANAGEMENT()
{
    outputchampsimstatistics.remapping_request_queue_congestion = remapping_request_queue_congestion;
    delete& counter_table;
    delete& hotness_table;
    delete& line_location_table;
};

bool OS_TRANSPARENT_MANAGEMENT::memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree)
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

        // indicate the positions in line location table entry for address_in_fm and address_in_sm.
        remapping_request.fm_location = fm_location;
        remapping_request.sm_location = location;

        remapping_request.size = DATA_GRANULARITY_IN_CACHE_LINE;
        // check duplicated remapping request in remapping_request_queue
        // if duplicated remapping requests exist, we won't add this new remapping request into the remapping_request_queue.
        bool duplicated_remapping_request = false;
        for (uint64_t i = 0; i < remapping_request_queue.size(); i++)
        {
            uint64_t data_block_address_to_check = remapping_request_queue[i].address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
            uint64_t line_location_table_index_to_check = data_block_address_to_check % fast_memory_capacity_at_data_block_granularity;

            if (line_location_table_index_to_check == line_location_table_index)
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

void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(PACKET& packet)
{
    uint64_t data_block_address = packet.address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
    REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);

    uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
    uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

    REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

    packet.h_address = replace_bits(replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit), packet.address, DATA_MANAGEMENT_OFFSET_BITS - 1);

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
    packet.h_address_fm = replace_bits(replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(RemappingLocation::Zero) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit), packet.address, DATA_MANAGEMENT_OFFSET_BITS - 1);
#endif  // COLOCATED_LINE_LOCATION_TABLE
};

void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(uint64_t& address)
{
    uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
    REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);

    uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
    uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

    REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

    address = replace_bits(replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit), address, DATA_MANAGEMENT_OFFSET_BITS - 1);
};

bool OS_TRANSPARENT_MANAGEMENT::issue_remapping_request(RemappingRequest& remapping_request)
{
    if (remapping_request_queue.empty() == false)
    {
        remapping_request = remapping_request_queue.front();
        return true;
    }

    return false;
};

bool OS_TRANSPARENT_MANAGEMENT::finish_remapping_request()
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

void OS_TRANSPARENT_MANAGEMENT::cold_data_detection()
{
    cycle++;
}

bool OS_TRANSPARENT_MANAGEMENT::cold_data_eviction(uint64_t source_address, float queue_busy_degree)
{
    return false;
}

bool OS_TRANSPARENT_MANAGEMENT::enqueue_remapping_request(RemappingRequest& remapping_request)
{
    return false;
}

#if (COLOCATED_LINE_LOCATION_TABLE == ENABLE)
bool OS_TRANSPARENT_MANAGEMENT::finish_fm_access_in_incomplete_read_request_queue(uint64_t h_address)
{
    for (size_t i = 0; i < incomplete_read_request_queue.size(); i++)
    {
        if (incomplete_read_request_queue[i].packet.h_address == h_address)
        {
            if (incomplete_read_request_queue[i].fm_access_finish == false)
            {
                incomplete_read_request_queue[i].fm_access_finish = true;
                return true;
            }
            else
            {
                continue;
            }
        }
    }

    return false;
}

bool OS_TRANSPARENT_MANAGEMENT::finish_fm_access_in_incomplete_write_request_queue(uint64_t h_address)
{
    for (size_t i = 0; i < incomplete_write_request_queue.size(); i++)
    {
        if (incomplete_write_request_queue[i].packet.h_address == h_address)
        {
            if (incomplete_write_request_queue[i].fm_access_finish == false)
            {
                incomplete_write_request_queue[i].fm_access_finish = true;
                return true;
            }
            else
            {
                continue;
            }
        }
    }

    return false;
}
#endif  // COLOCATED_LINE_LOCATION_TABLE

#endif  // IDEAL_LINE_LOCATION_TABLE, COLOCATED_LINE_LOCATION_TABLE
#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

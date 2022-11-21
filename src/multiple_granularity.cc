#include "os_transparent_management.h"

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_MULTIPLE_GRANULARITY == ENABLE)
OS_TRANSPARENT_MANAGEMENT::OS_TRANSPARENT_MANAGEMENT(COUNTER_WIDTH threshold, uint64_t max_address, uint64_t fast_memory_max_address)
    : hotness_threshold(threshold), total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
    total_capacity_at_data_block_granularity(max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_capacity_at_data_block_granularity(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_offset_bit(lg2(fast_memory_max_address)),   // note here only support integers of 2's power.
    counter_table(*(new std::vector<COUNTER_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, COUNTER_DEFAULT_VALUE))),
    hotness_table(*(new std::vector<HOTNESS_WIDTH>(max_address >> DATA_MANAGEMENT_OFFSET_BITS, HOTNESS_DEFAULT_VALUE))),
    set_msb(REMAPPING_LOCATION_WIDTH_BITS + fast_memory_offset_bit - 1),
    access_table(*(new std::vector<AccessDistribution>(max_address >> DATA_MANAGEMENT_OFFSET_BITS))),
    placement_table(*(new std::vector<PlacementEntry>(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS)))
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
    delete& access_table;
    delete& placement_table;
};

bool OS_TRANSPARENT_MANAGEMENT::memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree)
{
    if (address >= total_capacity)
    {
        std::cout << __func__ << ": address input error." << std::endl;
        return false;
    }

    uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in placement table
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the block number of the data block address
    uint64_t first_address = data_block_address << DATA_MANAGEMENT_OFFSET_BITS; // the first address in the page of this address
    START_ADDRESS_WIDTH data_line_positon = START_ADDRESS_WIDTH((address >> DATA_LINE_OFFSET_BITS) - (first_address >> DATA_LINE_OFFSET_BITS));

    // mark accessed data line
    access_table.at(data_block_address).access[data_line_positon] = true;

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

    // prepare a remapping request
    RemappingRequest remapping_request;

    // this data block is hot and belongs to slow memory
    if ((hotness_table.at(data_block_address) == true) && (tag != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
    {
        // check whether there have enough invalid groups in placement table entry for new migration
        if (placement_table.at(placement_table_index).cursor >= NUMBER_OF_BLOCK)
        {
            // no enough invalid groups for new migration
            return true;
        }

        // calculate the free space in fast memory (this action can be optimized to [bool is_migrated = false;])
        MIGRATION_GRANULARITY_WIDTH free_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to calculate the free space in fast memory.
            free_space -= placement_table.at(placement_table_index).granularity[i];
        }

        if (free_space == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
        {
            // no enough free space for new migration
            return true;
        }

        // if (free_space > MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4))
        // {
        //     std::cout << __func__ << ": free_space calculation error." << std::endl;
        //     assert(0);
        // }

        // calculate the start address
        START_ADDRESS_WIDTH start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
        for (START_ADDRESS_WIDTH i = START_ADDRESS_WIDTH(StartAddress::Zero); i < START_ADDRESS_WIDTH(StartAddress::Max); i++)
        {
            if (access_table.at(data_block_address).access[i] == true)
            {
                start_address = i;
                break;
            }
        }

        // calculate the end address
        START_ADDRESS_WIDTH end_address = START_ADDRESS_WIDTH(StartAddress::Zero);
        for (START_ADDRESS_WIDTH i = START_ADDRESS_WIDTH(StartAddress::Max) - 1; i > START_ADDRESS_WIDTH(StartAddress::Zero); i--)
        {
            if (access_table.at(data_block_address).access[i] == true)
            {
                end_address = i;
                break;
            }
        }

        MIGRATION_GRANULARITY_WIDTH migration_granularity = calculate_migration_granularity(start_address, end_address);

        // check whether this migration granularity is beyond the block's range
        while (true)
        {
            if ((start_address + migration_granularity - 1) >= START_ADDRESS_WIDTH(StartAddress::Max))
            {
                if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2);
                }
                else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1);
                }
                else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512);
                }
                else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256);
                }
                else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128);
                }
                else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_64) < migration_granularity) && (migration_granularity <= MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128)))
                {
                    migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_64);
                }
                else
                {
                    std::cout << __func__ << ": migration granularity calculation error." << std::endl;
                    assert(0);
                }
            }
            else
            {
                // this migration granularity is within the block's range.
                end_address = start_address + migration_granularity - 1;
                break;
            }
        }

        // check placement metadata to find if this data block is already in fast memory
        bool is_migrated = false;
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to find its block number if it exists.
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                is_migrated = true;
                data_block_position = i;
                break;
            }
        }

        if (is_migrated)    // this data block is already in fast memory
        {
            // check whether this existing group can be expanded
            if ((placement_table.at(placement_table_index).cursor - 1) == data_block_position)
            {
                if (placement_table.at(placement_table_index).start_address[data_block_position] == start_address)  // their start addresses are same
                {
                    if (placement_table.at(placement_table_index).granularity[data_block_position] < migration_granularity)
                    {
                        MIGRATION_GRANULARITY_WIDTH remain_hot_data = migration_granularity - placement_table.at(placement_table_index).granularity[data_block_position];
                        if (remain_hot_data <= free_space)
                        {
                            migration_granularity = remain_hot_data;

                            // this should be equal to placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position]
                            start_address = (end_address + 1) - remain_hot_data;
                        }
                        else
                        {
                            // no enough free space for data migration. Data eviction is necessary.
                            cold_data_eviction(address, queue_busy_degree);
                            return true;
                        }
                    }
                    else
                    {
                        // not need to migrate hot data because the data are already in fast memory. (hit in fast memory)
                        return true;
                    }
                }
                else
                {
                    // this existing group can not be expanded because their start addresses are different (to reduce complexity)
                    return true;
                }
            }
            else
            {
                // this existing group can not be expanded because no invalid groups behind it (i.e., no continuous free space). Data eviction is necessary.
                cold_data_eviction(address, queue_busy_degree);
                return true;
            }
        }
        else
        {
            // this data block is new to the placement table entry.
            if (migration_granularity <= free_space)
            {
                /* code */
                // no code here currently
            }
            else
            {
                // no enough free space for data migration. Data eviction is necessary.
                cold_data_eviction(address, queue_busy_degree);
                return true;
            }
        }

        // this should be RemappingLocation::Zero.
        REMAPPING_LOCATION_WIDTH fm_location = placement_table.at(placement_table_index).tag[placement_table.at(placement_table_index).cursor];
        if (fm_location != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
            std::cout << __func__ << ": fm_location calculation error." << std::endl;
            assert(0);
        }
        // follow rule 2 (data blocks belonging to NM are recovered to the original locations)
        START_ADDRESS_WIDTH start_address_in_fm = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4) - free_space;

        remapping_request.address_in_fm = replace_bits(base_remapping_address + (start_address_in_fm << DATA_LINE_OFFSET_BITS), uint64_t(fm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
        remapping_request.address_in_sm = replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(tag) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);

        // indicate where the data come from for address_in_fm and address_in_sm. (What block the data belong to)
        remapping_request.fm_location = fm_location;    // this should be RemappingLocation::Zero.
        remapping_request.sm_location = tag;            // this shouldn't be RemappingLocation::Zero.

        remapping_request.size = migration_granularity;
    }
    else if ((hotness_table.at(data_block_address) == false) && (tag != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
    {
        // this data block is cold and belongs to slow memory

        // check placement metadata to find if this data block is already in fast memory
        bool is_migrated = false;
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to find its block number if it exists.
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                is_migrated = true;
                data_block_position = i;
                break;
            }
        }

        if (is_migrated)    // this data block is already in fast memory
        {
            if ((placement_table.at(placement_table_index).start_address[data_block_position] <= data_line_positon) && (data_line_positon < (placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position])))
            {
                // hit in fast memory
            }
            else
            {
                // hit in slow memory
                cold_data_eviction(address, queue_busy_degree);
            }
        }
        else
        {
            // hit in slow memory
            cold_data_eviction(address, queue_busy_degree);
        }
        return true;
    }
    else if (tag == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)) // this data block belongs to fast memory.
    {
        // calculate the start address
        START_ADDRESS_WIDTH start_address = data_line_positon;

        // check whether this cache line is already in fast memory.
        bool in_fm = true;
        REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            static START_ADDRESS_WIDTH accumulated_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            accumulated_group_end_address += placement_table.at(placement_table_index).granularity[i] - 1;
            used_space += placement_table.at(placement_table_index).granularity[i];

            /*if (placement_table.at(placement_table_index).granularity[i] != MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
            {
                accumulated_group_end_address += placement_table.at(placement_table_index).granularity[i] - 1;
                used_space += placement_table.at(placement_table_index).granularity[i];
            }
            else
            {
                // arrive the invalid group (assume the groups after the first invalid group are all invalid)
                // this cache line is already in fast memory
                in_fm = true;
                break;
            }*/

            if (start_address <= accumulated_group_end_address)
            {
                if (placement_table.at(placement_table_index).tag[i] == tag)
                {
                    // this cache line is already in fast memory
                    in_fm = true;
                    break;
                }
                else
                {
                    // this cache line is in slow memory
                    in_fm = false;
                    occupied_group_number = i;
                    used_space -= placement_table.at(placement_table_index).granularity[i];
                    break;
                }
            }
        }

        if (in_fm)
        {
            // no need for migration
            return true;
        }
        else
        {
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[occupied_group_number];

            // follow rule 2 (data blocks belonging to NM are recovered to the original locations)
            START_ADDRESS_WIDTH start_address_in_fm = used_space;
            start_address = placement_table.at(placement_table_index).start_address[occupied_group_number];

            remapping_request.address_in_fm = replace_bits(base_remapping_address + (start_address_in_fm << DATA_LINE_OFFSET_BITS), uint64_t(tag) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
            remapping_request.address_in_sm = replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);

            // indicate where the data come from for address_in_fm and address_in_sm. (What block the data belong to)
            remapping_request.fm_location = sm_location;    // this shouldn't be RemappingLocation::Zero.
            remapping_request.sm_location = tag;            // this should be RemappingLocation::Zero.

            remapping_request.size = placement_table.at(placement_table_index).granularity[occupied_group_number];
        }
    }
    else
    {
        return true;
    }

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

    // add new remapping request to queue
    if ((duplicated_remapping_request == false) && (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD))
    {
        if (remapping_request_queue.size() < REMAPPING_REQUEST_QUEUE_LENGTH)
        {
            if (remapping_request.address_in_fm == remapping_request.address_in_sm)    // check
            {
                std::cout << __func__ << ": add new remapping request error." << std::endl;
                abort();
            }

            // enqueue a remapping request
            remapping_request_queue.push_back(remapping_request);
        }
        else
        {
            remapping_request_queue_congestion++;
        }
    }

    return true;
};

void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(PACKET& packet)
{
    uint64_t data_block_address = packet.address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);
    uint64_t first_address = data_block_address << DATA_MANAGEMENT_OFFSET_BITS;
    START_ADDRESS_WIDTH data_line_positon = START_ADDRESS_WIDTH((packet.address >> DATA_LINE_OFFSET_BITS) - (first_address >> DATA_LINE_OFFSET_BITS));

    if (tag != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
    {
        // this data block belongs to slow memory
        // check placement metadata to find if this data block is already in fast memory
        bool is_migrated = false;
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to find its block number if it exists.
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                is_migrated = true;
                data_block_position = i;
                break;
            }

            used_space += placement_table.at(placement_table_index).granularity[i];
        }

        if (is_migrated)
        {
            if ((placement_table.at(placement_table_index).start_address[data_block_position] <= data_line_positon) && (data_line_positon < (placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position])))
            {
                // the data of this physical address is in fast memory
                // calculate the start address
                START_ADDRESS_WIDTH start_address = used_space + data_line_positon - placement_table.at(placement_table_index).start_address[data_block_position];
                REMAPPING_LOCATION_WIDTH fm_location = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);
                packet.h_address = replace_bits(replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(fm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit), packet.address, DATA_LINE_OFFSET_BITS - 1);
            }
            else
            {
                // the data of this physical address is in slow memory, no need for translation
                packet.h_address = packet.address;
            }
        }
        else
        {
            // the data of this physical address is in slow memory, no need for translation
            packet.h_address = packet.address;
        }
    }
    else
    {
        // this data block belongs to fast memory
        // calculate the start address
        START_ADDRESS_WIDTH start_address = data_line_positon;

        // check whether this cache line is already in fast memory.
        bool in_fm = true;
        REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            static START_ADDRESS_WIDTH accumulated_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            accumulated_group_end_address += placement_table.at(placement_table_index).granularity[i] - 1;
            used_space += placement_table.at(placement_table_index).granularity[i];

            /*if (placement_table.at(placement_table_index).granularity[i] != MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
            {
                accumulated_group_end_address += placement_table.at(placement_table_index).granularity[i] - 1;
                used_space += placement_table.at(placement_table_index).granularity[i];
            }
            else
            {
                // arrive the invalid group (assume the groups after the first invalid group are all invalid)
                // this cache line is already in fast memory
                in_fm = true;
                break;
            }*/

            if (start_address <= accumulated_group_end_address)
            {
                if (placement_table.at(placement_table_index).tag[i] == tag)
                {
                    // this cache line is already in fast memory
                    in_fm = true;
                    break;
                }
                else
                {
                    // this cache line is in slow memory
                    in_fm = false;
                    occupied_group_number = i;
                    used_space -= placement_table.at(placement_table_index).granularity[i];
                    break;
                }
            }
        }

        if (in_fm)
        {
            // the data of this physical address is in fast memory, no need for translaton
            packet.h_address = packet.address;
        }
        else
        {
            // the data of this physical address is in slow memory
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[occupied_group_number];
            start_address = placement_table.at(placement_table_index).start_address[occupied_group_number] + start_address - used_space;

            packet.h_address = replace_bits(replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit), packet.address, DATA_LINE_OFFSET_BITS - 1);
        }
    }
};

void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(uint64_t& address)
{
    uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);
    uint64_t first_address = data_block_address << DATA_MANAGEMENT_OFFSET_BITS;
    START_ADDRESS_WIDTH data_line_positon = START_ADDRESS_WIDTH((address >> DATA_LINE_OFFSET_BITS) - (first_address >> DATA_LINE_OFFSET_BITS));

    if (tag != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
    {
        // this data block belongs to slow memory
        // check placement metadata to find if this data block is already in fast memory
        bool is_migrated = false;
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to find its block number if it exists.
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                is_migrated = true;
                data_block_position = i;
                break;
            }

            used_space += placement_table.at(placement_table_index).granularity[i];
        }

        if (is_migrated)
        {
            if ((placement_table.at(placement_table_index).start_address[data_block_position] <= data_line_positon) && (data_line_positon < (placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position])))
            {
                // the data of this physical address is in fast memory
                // calculate the start address
                START_ADDRESS_WIDTH start_address = used_space + data_line_positon - placement_table.at(placement_table_index).start_address[data_block_position];
                REMAPPING_LOCATION_WIDTH fm_location = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);
                address = replace_bits(replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(fm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit), address, DATA_LINE_OFFSET_BITS - 1);
            }
            else
            {
                // the data of this physical address is in slow memory, no need for translation
                address = address;
            }
        }
        else
        {
            // the data of this physical address is in slow memory, no need for translation
            address = address;
        }
    }
    else
    {
        // this data block belongs to fast memory
        // calculate the start address
        START_ADDRESS_WIDTH start_address = data_line_positon;

        // check whether this cache line is already in fast memory.
        bool in_fm = true;
        REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            static START_ADDRESS_WIDTH accumulated_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            accumulated_group_end_address += placement_table.at(placement_table_index).granularity[i] - 1;
            used_space += placement_table.at(placement_table_index).granularity[i];

            if (start_address <= accumulated_group_end_address)
            {
                if (placement_table.at(placement_table_index).tag[i] == tag)
                {
                    // this cache line is already in fast memory
                    in_fm = true;
                    break;
                }
                else
                {
                    // this cache line is in slow memory
                    in_fm = false;
                    occupied_group_number = i;
                    used_space -= placement_table.at(placement_table_index).granularity[i];
                    break;
                }
            }
        }

        if (in_fm)
        {
            // the data of this physical address is in fast memory, no need for translaton
            address = address;
        }
        else
        {
            // the data of this physical address is in slow memory
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[occupied_group_number];
            start_address = placement_table.at(placement_table_index).start_address[occupied_group_number] + start_address - used_space;

            address = replace_bits(replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit), address, DATA_LINE_OFFSET_BITS - 1);
        }
    }
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
        uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;

        // check whether the remapping_request moves block 0's data into fast memory
        if (remapping_request.fm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
            // this remapping_request moves block 0's data into slow memory

            data_block_address = remapping_request.address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;
            REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity);

            // check placement metadata to find if this data block is already in fast memory
            bool is_migrated = false;
            REMAPPING_LOCATION_WIDTH data_block_position = 0;
            for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
            {
                // traverse the placement entry to find its block number if it exists.
                if (placement_table.at(placement_table_index).tag[i] == tag)
                {
                    is_migrated = true;
                    data_block_position = i;
                    break;
                }
            }

            if (is_migrated)
            {
                // no code here
            }
            else
            {
                data_block_position = placement_table.at(placement_table_index).cursor;
            }

            // fill tag in placement table entry
            placement_table.at(placement_table_index).tag[data_block_position] = remapping_request.sm_location;

            START_ADDRESS_WIDTH start_address = (remapping_request.address_in_sm >> DATA_LINE_OFFSET_BITS) % (START_ADDRESS_WIDTH(StartAddress::Max));
            // fill start_address in placement table entry
            placement_table.at(placement_table_index).start_address[data_block_position] = start_address;

            // fill granularity in placement table entry
            placement_table.at(placement_table_index).granularity[data_block_position] = remapping_request.size;

            if (data_block_position == placement_table.at(placement_table_index).cursor)
            {
                // remember to update the cursor
                if (placement_table.at(placement_table_index).cursor < NUMBER_OF_BLOCK)
                {
                    placement_table.at(placement_table_index).cursor++;
                }
                else
                {
                    std::cout << __func__ << ": cursor calculation error." << std::endl;
                    assert(0);
                }
            }
        }
        else if (remapping_request.sm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
            // this remapping_request moves block 0's data into fast memory
            REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
            bool find_occupied_group = false;
            for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
            {
                if (placement_table.at(placement_table_index).tag[i] == remapping_request.fm_location)
                {
                    find_occupied_group = true;
                    occupied_group_number = i;
                    break;
                }
            }

            if (find_occupied_group == false)
            {
                std::cout << __func__ << ": occupied_group_number calculation error." << std::endl;
                assert(0);
            }
            // fill tag in placement table entry
            placement_table.at(placement_table_index).tag[occupied_group_number] = remapping_request.sm_location;

            START_ADDRESS_WIDTH start_address_in_fm = (remapping_request.address_in_fm >> DATA_LINE_OFFSET_BITS) % (START_ADDRESS_WIDTH(StartAddress::Max));
            // fill start_address in placement table entry
            placement_table.at(placement_table_index).start_address[occupied_group_number] = start_address_in_fm;

            // fill granularity in placement table entry
            placement_table.at(placement_table_index).granularity[occupied_group_number] = remapping_request.size;

            // check and mark invalid groups
            bool is_invalid = false;
            if ((occupied_group_number + 1) == placement_table.at(placement_table_index).cursor)
            {
                placement_table.at(placement_table_index).granularity[occupied_group_number] = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
                placement_table.at(placement_table_index).cursor = occupied_group_number;
                is_invalid = true;
            }

            if (is_invalid)
            {
                for (int8_t i = occupied_group_number - 1; i >= 0; i--) // go forward to check and mark invalid groups
                {
                    if (placement_table.at(placement_table_index).tag[i] == remapping_request.sm_location)
                    {
                        placement_table.at(placement_table_index).granularity[i] = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
                        placement_table.at(placement_table_index).cursor = i;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            std::cout << __func__ << ": fm_location calculation error." << std::endl;
            assert(0);
        }
    }
    else
    {
        std::cout << __func__ << ": remapping queue empty error." << std::endl;
        assert(0);
        return false;   // error
    }

    return true;
};

void OS_TRANSPARENT_MANAGEMENT::cold_data_detection()
{
    if ((cycle % INTERVAL_FOR_DECREMENT) == 0)
    {
        // Overhead here is heavy. OpenMP is necessary.
#if (USE_OPENMP == ENABLE)
#pragma omp parallel
        {
#pragma omp for
            for (uint64_t i = 0; i < total_capacity_at_data_block_granularity; i++)
            {
                counter_table[i] >>= 1; // halve the counter value
                if (counter_table[i] == 0)
                {
                    hotness_table.at(i) = false;    // mark cold data block
                    for (START_ADDRESS_WIDTH j = 0; j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
                    {
                        // clear accessed data line
                        access_table.at(i).access[j] = false;
                    }
                }
            }
        }
#else
        for (uint64_t i = 0; i < (total_capacity_at_data_block_granularity); i++)
        {
            counter_table[i] >>= 1; // halve the counter value
            if (counter_table[i] == 0)
            {
                hotness_table.at(i) = false;    // mark cold data block
                for (START_ADDRESS_WIDTH j = 0; j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
                {
                    // clear accessed data line
                    access_table.at(i).access[j] = false;
                }
            }
        }
#endif  // USE_OPENMP

    }

    cycle++;
}

bool OS_TRANSPARENT_MANAGEMENT::cold_data_eviction(uint64_t source_address, float queue_busy_degree)
{
#if (DATA_EVICTION == ENABLE)
    uint64_t data_block_address = source_address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in placement table
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the block number of the data block address
    //uint64_t first_address = data_block_address << DATA_MANAGEMENT_OFFSET_BITS;
    //START_ADDRESS_WIDTH data_line_positon = START_ADDRESS_WIDTH((source_address >> DATA_LINE_OFFSET_BITS) - (first_address >> DATA_LINE_OFFSET_BITS));

    // find cold data block and it belongs to slow memory in placement table entry
    bool is_cold = false;
    REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
    MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
    for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
    {
        if (placement_table.at(placement_table_index).granularity[i] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
        {
            std::cout << __func__ << ": used space calculation error." << std::endl;
            assert(0);
        }

        used_space += placement_table.at(placement_table_index).granularity[i];

        if (placement_table.at(placement_table_index).tag[i] == tag)
        {
            // don't evict the data block which has same block number.
            continue;
        }
        else if (placement_table.at(placement_table_index).tag[i] != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
            // check whether this data block is cold
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[i];
            uint64_t data_base_address_to_evict = replace_bits(base_remapping_address, uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
            uint64_t data_block_address_to_evict = data_base_address_to_evict >> DATA_MANAGEMENT_OFFSET_BITS;

            if (hotness_table.at(data_block_address_to_evict) == false) // this data block is cold
            {
                is_cold = true;
                occupied_group_number = i;
                used_space -= placement_table.at(placement_table_index).granularity[i];
                break;
            }
        }
    }

    if (is_cold)
    {
        REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[occupied_group_number];

        // follow rule 2 (data blocks belonging to NM are recovered to the original locations)
        START_ADDRESS_WIDTH start_address_in_fm = used_space;
        START_ADDRESS_WIDTH start_address = placement_table.at(placement_table_index).start_address[occupied_group_number];

        tag = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);

        // prepare a remapping request
        RemappingRequest remapping_request;
        remapping_request.address_in_fm = replace_bits(base_remapping_address + (start_address_in_fm << DATA_LINE_OFFSET_BITS), uint64_t(tag) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
        remapping_request.address_in_sm = replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);

        // indicate where the data come from for address_in_fm and address_in_sm.
        remapping_request.fm_location = sm_location;    // this shouldn't be RemappingLocation::Zero.
        remapping_request.sm_location = tag;            // this should be RemappingLocation::Zero.

        remapping_request.size = placement_table.at(placement_table_index).granularity[occupied_group_number];

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

        // add new remapping request to queue
        if ((duplicated_remapping_request == false) && (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD))
        {
            if (remapping_request_queue.size() < REMAPPING_REQUEST_QUEUE_LENGTH)
            {
                if (remapping_request.address_in_fm == remapping_request.address_in_sm)    // check
                {
                    std::cout << __func__ << ": add new remapping request error." << std::endl;
                    abort();
                }

                // enqueue a remapping request
                remapping_request_queue.push_back(remapping_request);
            }
            else
            {
                remapping_request_queue_congestion++;
            }
        }
        else
        {
            return false;
        }
        // new eviction request is issued.
        return true;
    }
#endif  // DATA_EVICTION

    return false;
}

MIGRATION_GRANULARITY_WIDTH OS_TRANSPARENT_MANAGEMENT::calculate_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address)
{
    // check
    if (start_address > end_address)
    {
        std::cout << __func__ << ": migration granularity calculation error." << std::endl;
        assert(0);
    }

    // calculate the migration granularity
    MIGRATION_GRANULARITY_WIDTH migration_granularity = end_address - start_address;

    if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_64)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_64);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_64) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2);
    }
    else if ((MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2) <= migration_granularity) && (migration_granularity < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4)))
    {
        migration_granularity = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4);
    }
    else
    {
        std::cout << __func__ << ": migration granularity calculation error 2." << std::endl;
        assert(0);
    }

    return migration_granularity;
}

#endif  // IDEAL_MULTIPLE_GRANULARITY
#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

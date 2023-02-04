#include "os_transparent_management.h"

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_VARIABLE_GRANULARITY == ENABLE)
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

    expected_number_in_congruence_group = total_capacity / fast_memory_capacity;
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

#if (STATISTICS_INFORMATION == ENABLE)
    uint64_t estimated_spatial_locality_counts[MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max)] = {0};
    uint64_t estimated_spatial_locality_total_counts = 0;
    uint64_t granularity_counts[MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max)] = {0};
    uint64_t granularity_total_counts = 0;
    uint64_t granularity_predict_counts[MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max)] = {0};
    uint64_t granularity_total_predict_counts = 0;
    uint64_t access_table_size = total_capacity >> DATA_MANAGEMENT_OFFSET_BITS;

    for (uint64_t i = 0; i < access_table_size; i++)
    {
        if (access_table.at(i).access_flag == true)
        {
            // calculate the start address
            START_ADDRESS_WIDTH start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            for (START_ADDRESS_WIDTH j = START_ADDRESS_WIDTH(StartAddress::Zero); j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
            {
                if (access_table.at(i).access_stats[j] == true)
                {
                    start_address = j;
                    break;
                }
            }

            // calculate the end address
            START_ADDRESS_WIDTH end_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            for (START_ADDRESS_WIDTH j = START_ADDRESS_WIDTH(StartAddress::Max) - 1; j > START_ADDRESS_WIDTH(StartAddress::Zero); j--)
            {
                if (access_table.at(i).access_stats[j] == true)
                {
                    end_address = j;
                    break;
                }
            }

            estimated_spatial_locality_counts[access_table.at(i).estimated_spatial_locality_stats]++;

            access_table.at(i).granularity_stats = end_address - start_address + 1;
            granularity_counts[access_table.at(i).granularity_stats]++;

            granularity_predict_counts[access_table.at(i).granularity_predict_stats]++;
        }
    }

    fprintf(outputchampsimstatistics.trace_file, "\n\nInformation about variable granularity\n\n");

    // print out spatial locality
    fprintf(outputchampsimstatistics.trace_file, "Estimated spatial locality distribution:\n");
    for (MIGRATION_GRANULARITY_WIDTH i = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None); i < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max); i++)
    {
        fprintf(outputchampsimstatistics.trace_file, "Spatial locality [%d] %ld\n", i, estimated_spatial_locality_counts[i]);
        estimated_spatial_locality_total_counts += estimated_spatial_locality_counts[i];
    }
    fprintf(outputchampsimstatistics.trace_file, "estimated_spatial_locality_total_counts %ld\n", estimated_spatial_locality_total_counts);

    // print out best granularity
    fprintf(outputchampsimstatistics.trace_file, "\nBest granularity distribution:\n");
    for (MIGRATION_GRANULARITY_WIDTH i = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None); i < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max); i++)
    {
        fprintf(outputchampsimstatistics.trace_file, "Granularity [%d] %ld\n", i, granularity_counts[i]);
        granularity_total_counts += granularity_counts[i];
    }
    fprintf(outputchampsimstatistics.trace_file, "granularity_total_counts %ld\n", granularity_total_counts);

    fprintf(outputchampsimstatistics.trace_file, "\nPredicted granularity distribution:\n");
    for (MIGRATION_GRANULARITY_WIDTH i = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None); i < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Max); i++)
    {
        fprintf(outputchampsimstatistics.trace_file, "Granularity [%d] %ld\n", i, granularity_predict_counts[i]);
        granularity_total_predict_counts += granularity_predict_counts[i];
    }
    fprintf(outputchampsimstatistics.trace_file, "granularity_total_predict_counts %ld\n", granularity_total_predict_counts);

#endif  // STATISTICS_INFORMATION

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
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the tag of the data block address
    uint64_t first_address = data_block_address << DATA_MANAGEMENT_OFFSET_BITS; // the first address in the page granularity of this address
    START_ADDRESS_WIDTH data_line_positon = START_ADDRESS_WIDTH((address >> DATA_LINE_OFFSET_BITS) - (first_address >> DATA_LINE_OFFSET_BITS));

    // mark accessed data line
    access_table.at(data_block_address).access[data_line_positon] = true;

#if (STATISTICS_INFORMATION == ENABLE)
    access_table.at(data_block_address).access_flag = true;
    access_table.at(data_block_address).access_stats[data_line_positon] = true;
    access_table.at(data_block_address).temporal_access_stats[data_line_positon] = true;
#endif  // STATISTICS_INFORMATION

#if (COLD_DATA_DETECTION_IN_GROUP == ENABLE)
    cold_data_detection_in_group(address);
#endif  // COLD_DATA_DETECTION_IN_GROUP

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
        // calculate the free space in fast memory for this set
        int16_t free_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // traverse the placement entry to calculate the free space in fast memory.
            free_space -= placement_table.at(placement_table_index).granularity[i];
        }

        // sanity check
        if (free_space < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
        {
            std::cout << __func__ << ": free_space calculation error." << std::endl;
            for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
            {
                printf("tag[%d]: %d, ", i, placement_table.at(placement_table_index).tag[i]);
                printf("start_address[%d]: %d, ", i, placement_table.at(placement_table_index).start_address[i]);
                printf("granularity[%d]: %d.\n", i, placement_table.at(placement_table_index).granularity[i]);
            }
            assert(0);
        }

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

        end_address = adjust_migration_granularity(start_address, end_address, migration_granularity);

        // check placement metadata to find if this data block is already in fast memory
        bool is_expanded = false;
        bool is_limited = false; // existing groups could limit end_address and/or start_address
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            REMAPPING_LOCATION_WIDTH data_block_position = 0;
            START_ADDRESS_WIDTH existing_group_start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            START_ADDRESS_WIDTH existing_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            // traverse the placement entry to find its tag if it exists. (checking multiple groups with same tag is possible)
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                // record the information of this existing group
                data_block_position = i;
                existing_group_start_address = placement_table.at(placement_table_index).start_address[data_block_position];
                existing_group_end_address = placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position] - 1;

                // check the position of this existing group, note here the cursor won't be zero
                if ((placement_table.at(placement_table_index).cursor - 1) != data_block_position)
                {
#if (FLEXIBLE_DATA_PLACEMENT == ENABLE)
                    if ((start_address <= existing_group_end_address) && (existing_group_end_address < end_address))
                    {
                        // case one: the front of new hot data is overlapped with this existing group
                        start_address = existing_group_end_address + 1;
                        if (is_limited)
                        {
                            end_address = round_down_migration_granularity(start_address, end_address, migration_granularity);
                        }
                        else
                        {
                            end_address = adjust_migration_granularity(start_address, end_address, migration_granularity);
                        }
                    }
                    else if ((start_address < existing_group_start_address) && (existing_group_end_address < end_address))
                    {
                        // case two: new hot data contain the data of this existing group 
                        is_limited = true;
                        end_address = existing_group_start_address - 1; // throw the rear of the new hot data

                        end_address = round_down_migration_granularity(start_address, end_address, migration_granularity);
                    }
                    else if ((existing_group_start_address <= start_address) && (end_address <= existing_group_end_address))
                    {
                        // case three: hit in fast memory
                        return true;
                    }
                    else if ((start_address < existing_group_start_address) && (existing_group_start_address <= end_address))
                    {
                        // case four: the rear of new hot data is overlapped with this existing group
                        is_limited = true;
                        end_address = existing_group_start_address - 1; // throw the rear of the new hot data

                        end_address = round_down_migration_granularity(start_address, end_address, migration_granularity);
                    }
                    else
                    {
                        // case five: no data overlapping
                    }

                    // this existing group can not be expanded because no invalid groups behind it (i.e., no continuous free space).
                    outputchampsimstatistics.unexpandable_since_no_invalid_group++;
#else
                    // this existing group can not be expanded because no invalid groups behind it (i.e., no continuous free space).
                    cold_data_eviction(address, queue_busy_degree);
                    outputchampsimstatistics.unexpandable_since_no_invalid_group++;
                    return true;
#endif  // FLEXIBLE_DATA_PLACEMENT

                }
                else
                {
                    // right in the front of the position pointed by the cursor

                    if (start_address < existing_group_start_address)
                    {
#if (FLEXIBLE_DATA_PLACEMENT == ENABLE)
                        // case one: this existing group can't be expanded
                        is_limited = true;
                        end_address = existing_group_start_address - 1; // throw the rear of the new hot data

                        end_address = round_down_migration_granularity(start_address, end_address, migration_granularity);
                        break;
#else
                        // this existing group can not be expanded because the new start_address is smaller than the existing group's start_address
                        outputchampsimstatistics.unexpandable_since_start_address++;
                        return true;
#endif  // FLEXIBLE_DATA_PLACEMENT
                    }
                    else
                    {
                        // case two: this existing group can be expanded
                        // synchronize their start addresses
                        start_address = existing_group_start_address;

                        if (existing_group_end_address < end_address)
                        {
                            // new hot data are needed to migrate
                            // check whether this new migration_granularity is possible to track in the remapping table
                            migration_granularity = calculate_migration_granularity(start_address, end_address);

                            if (is_limited)
                            {
                                end_address = round_down_migration_granularity(start_address, end_address, migration_granularity);
                            }
                            else
                            {
                                end_address = adjust_migration_granularity(start_address, end_address, migration_granularity);
                            }

                            if (placement_table.at(placement_table_index).granularity[data_block_position] < migration_granularity)
                            {
                                MIGRATION_GRANULARITY_WIDTH remain_hot_data = migration_granularity - placement_table.at(placement_table_index).granularity[data_block_position];

                                // chech whether there has enough free space
                                if (remain_hot_data <= free_space)
                                {
                                    // this existing group is needed to be expanded
                                    is_expanded = true;
                                    migration_granularity = remain_hot_data;

                                    // this should be equal to placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position]
                                    start_address = (end_address + 1) - remain_hot_data;
                                    break;
                                }
                                else
                                {
#if (FLEXIBLE_GRANULARITY == ENABLE)
                                    if (free_space)
                                    {
                                        // free_space is not empty
                                        // this existing group is needed to be expanded
                                        is_expanded = true;
                                        migration_granularity = free_space;

                                        // this should be equal to placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position]
                                        start_address = (end_address + 1) - free_space;
                                        break;
                                    }
                                    else
                                    {
                                        // no enough free space for data migration. Data eviction is necessary.
                                        cold_data_eviction(address, queue_busy_degree);
                                        outputchampsimstatistics.no_free_space_for_migration++;
                                        return true;
                                    }

#else
                                    // no enough free space for data migration. Data eviction is necessary.
                                    cold_data_eviction(address, queue_busy_degree);
                                    outputchampsimstatistics.no_free_space_for_migration++;
                                    return true;
#endif  // FLEXIBLE_GRANULARITY
                                }
                            }
                            else
                            {
                                // not need to migrate hot data because no need to update the migration_granularity. (hit in fast/slow memory)
                                return true;
                            }
                        }
                        else
                        {
                            // not need to migrate hot data because the data are already in fast memory. (hit in fast memory)
                            return true;
                        }
                    }
                }
            }
        }

        // sanity check
        if (migration_granularity == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
        {
            std::cout << __func__ << ": migration granularity calculation error." << std::endl;
            assert(0);
        }

        if (is_expanded)    // this data block can be expanded in fast memory
        {
            // no need to chech whether there has enough free space
            // no need to check the cursor
        }
        else
        {
            // this data block can not be expanded in fast memory (new to or part of it is in fast memory)
            // chech whether there has enough free space
            if (migration_granularity <= free_space)
            {
                // check whether there have enough invalid groups in placement table entry for new migration
                if (placement_table.at(placement_table_index).cursor == NUMBER_OF_BLOCK)
                {
                    // no enough invalid groups for data migration. Data eviction is necessary.
                    cold_data_eviction(address, queue_busy_degree);
                    outputchampsimstatistics.no_invalid_group_for_migration++;
                    return true;
                }
            }
            else
            {
#if (FLEXIBLE_GRANULARITY == ENABLE)
                if (free_space)
                {
                    // free_space is not empty
                    // check whether there have enough invalid groups in placement table entry for new migration
                    if (placement_table.at(placement_table_index).cursor == NUMBER_OF_BLOCK)
                    {
                        // no enough invalid groups for data migration. Data eviction is necessary.
                        cold_data_eviction(address, queue_busy_degree);
                        outputchampsimstatistics.no_invalid_group_for_migration++;
                        return true;
                    }
                    migration_granularity = free_space;
                }
                else
                {
                    // no enough free space for data migration. Data eviction is necessary.
                    cold_data_eviction(address, queue_busy_degree);
                    outputchampsimstatistics.no_free_space_for_migration++;
                    return true;
                }
#else
                // no enough free space for data migration. Data eviction is necessary.
                cold_data_eviction(address, queue_busy_degree);
                outputchampsimstatistics.no_free_space_for_migration++;
                return true;
#endif  // FLEXIBLE_GRANULARITY
            }
        }

        // this should be RemappingLocation::Zero.
        REMAPPING_LOCATION_WIDTH fm_location = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);

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

        // check placement metadata to find if the data of this data block is already in fast memory
        bool is_hit = false;    // hit in fast memory
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            REMAPPING_LOCATION_WIDTH data_block_position = 0;
            START_ADDRESS_WIDTH existing_group_start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            START_ADDRESS_WIDTH existing_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            // traverse the placement entry to find its tag if it exists. (checking multiple groups with same tag is possible)
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                // record the information of this existing group
                data_block_position = i;
                existing_group_start_address = placement_table.at(placement_table_index).start_address[data_block_position];
                existing_group_end_address = placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position] - 1;

                if ((existing_group_start_address <= data_line_positon) && (data_line_positon <= existing_group_end_address))
                {
                    // hit in fast memory
                    is_hit = true;
                    break;
                }
            }
        }

        if (is_hit)    // the data of this data block is already in fast memory
        {
            // hit in fast memory
            /* no code here */
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
            used_space += placement_table.at(placement_table_index).granularity[i];
            START_ADDRESS_WIDTH accumulated_group_end_address = used_space - 1;

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
                    used_space -= placement_table.at(placement_table_index).granularity[occupied_group_number];
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

    if (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD)
    {
        enqueue_remapping_request(remapping_request);
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
        // check placement metadata to find if the data of this data block is already in fast memory
        bool is_hit = false;    // hit in fast memory
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            START_ADDRESS_WIDTH existing_group_start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            START_ADDRESS_WIDTH existing_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            // traverse the placement entry to find its tag if it exists. (checking multiple groups with same tag is possible)
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                // record the information of this existing group
                data_block_position = i;
                existing_group_start_address = placement_table.at(placement_table_index).start_address[data_block_position];
                existing_group_end_address = placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position] - 1;

                if ((existing_group_start_address <= data_line_positon) && (data_line_positon <= existing_group_end_address))
                {
                    // hit in fast memory
                    is_hit = true;
                    break;
                }
            }

            used_space += placement_table.at(placement_table_index).granularity[i];
        }

        if (is_hit)
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
        // this data block belongs to fast memory
        // calculate the start address
        START_ADDRESS_WIDTH start_address = data_line_positon;

        // check whether this cache line is already in fast memory.
        bool in_fm = true;
        REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            used_space += placement_table.at(placement_table_index).granularity[i];
            START_ADDRESS_WIDTH accumulated_group_end_address = used_space - 1;

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
                    used_space -= placement_table.at(placement_table_index).granularity[occupied_group_number];
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
        // check placement metadata to find if the data of this data block is already in fast memory
        bool is_hit = false;    // hit in fast memory
        REMAPPING_LOCATION_WIDTH data_block_position = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            START_ADDRESS_WIDTH existing_group_start_address = START_ADDRESS_WIDTH(StartAddress::Zero);
            START_ADDRESS_WIDTH existing_group_end_address = START_ADDRESS_WIDTH(StartAddress::Zero);

            // traverse the placement entry to find its tag if it exists. (checking multiple groups with same tag is possible)
            if (placement_table.at(placement_table_index).tag[i] == tag)
            {
                // record the information of this existing group
                data_block_position = i;
                existing_group_start_address = placement_table.at(placement_table_index).start_address[data_block_position];
                existing_group_end_address = placement_table.at(placement_table_index).start_address[data_block_position] + placement_table.at(placement_table_index).granularity[data_block_position] - 1;

                if ((existing_group_start_address <= data_line_positon) && (data_line_positon <= existing_group_end_address))
                {
                    // hit in fast memory
                    is_hit = true;
                    break;
                }
            }

            used_space += placement_table.at(placement_table_index).granularity[i];
        }

        if (is_hit)
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
        // this data block belongs to fast memory
        // calculate the start address
        START_ADDRESS_WIDTH start_address = data_line_positon;

        // check whether this cache line is already in fast memory.
        bool in_fm = true;
        REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
        MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
        for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
        {
            used_space += placement_table.at(placement_table_index).granularity[i];
            START_ADDRESS_WIDTH accumulated_group_end_address = used_space - 1;

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
                    used_space -= placement_table.at(placement_table_index).granularity[occupied_group_number];
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
            REMAPPING_LOCATION_WIDTH tag = remapping_request.sm_location;
            START_ADDRESS_WIDTH start_address = (remapping_request.address_in_sm >> DATA_LINE_OFFSET_BITS) % (START_ADDRESS_WIDTH(StartAddress::Max));

            // check whether part of this data block can be expanded in fast memory
            REMAPPING_LOCATION_WIDTH data_block_position = 0;
            bool is_expanded = false;
            if (placement_table.at(placement_table_index).cursor > 0)
            {
                if (placement_table.at(placement_table_index).tag[placement_table.at(placement_table_index).cursor - 1] == tag)
                {
                    // record the information of this existing group
                    data_block_position = placement_table.at(placement_table_index).cursor - 1;
                    START_ADDRESS_WIDTH existing_group_start_address = placement_table.at(placement_table_index).start_address[data_block_position];

                    if (existing_group_start_address <= start_address)
                    {
                        is_expanded = true;
                    }
                    else
                    {
#if (FLEXIBLE_DATA_PLACEMENT == ENABLE)
#else
                        std::cout << __func__ << ": start address calculation error." << std::endl;
                        printf("existing_group_start_address: %d, ", existing_group_start_address);
                        printf("start_address: %d.\n", start_address);
                        assert(0);
#endif  // FLEXIBLE_DATA_PLACEMENT
                    }
                }
            }

            if (is_expanded)
            {
                // expand this existing group

#if (STATISTICS_INFORMATION == ENABLE)
                if (placement_table.at(placement_table_index).granularity[data_block_position] == access_table.at(data_block_address).granularity_predict_stats)
                {
                    access_table.at(data_block_address).granularity_predict_stats += remapping_request.size;
                }
#endif  // STATISTICS_INFORMATION

                // fill granularity in placement table entry
                placement_table.at(placement_table_index).granularity[data_block_position] += remapping_request.size;

#if (FLEXIBLE_GRANULARITY == ENABLE)
#else
                // sanity check
                if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_128))
                {
                    /* code */
                }
                else if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_256))
                {
                    /* code */
                }
                else if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::Byte_512))
                {
                    /* code */
                }
                else if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_1))
                {
                    /* code */
                }
                else if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_2))
                {
                    /* code */
                }
                else if (placement_table.at(placement_table_index).granularity[data_block_position] == MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4))
                {
                    /* code */
                }
                else
                {
                    printf("placement_table.at(%ld).granularity[%d] is %d.\n", placement_table_index, data_block_position, placement_table.at(placement_table_index).granularity[data_block_position]);
                    printf("remapping_request.size is %d.\n", remapping_request.size);
                    printf("tag is %d.\n", tag);
                    assert(0);
                }
#endif  // FLEXIBLE_GRANULARITY
            }
            else
            {
#if (STATISTICS_INFORMATION == ENABLE)
                if (remapping_request.size > access_table.at(data_block_address).granularity_predict_stats)
                {
                    // record the biggest predicted granularity
                    access_table.at(data_block_address).granularity_predict_stats = remapping_request.size;
                }
#endif  // STATISTICS_INFORMATION

                data_block_position = placement_table.at(placement_table_index).cursor;

                // fill tag in placement table entry
                placement_table.at(placement_table_index).tag[data_block_position] = remapping_request.sm_location;

                // fill start_address in placement table entry
                placement_table.at(placement_table_index).start_address[data_block_position] = start_address;

                // fill granularity in placement table entry
                placement_table.at(placement_table_index).granularity[data_block_position] = remapping_request.size;

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

                // calculate the free space in fast memory for this set
                int16_t free_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4);
                for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
                {
                    // traverse the placement entry to calculate the free space in fast memory.
                    free_space -= placement_table.at(placement_table_index).granularity[i];
                }

                // sanity check
                if (free_space < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
                {
                    std::cout << __func__ << ": free_space calculation error." << std::endl;
                    for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
                    {
                        printf("tag[%d]: %d, ", i, placement_table.at(placement_table_index).tag[i]);
                        printf("start_address[%d]: %d, ", i, placement_table.at(placement_table_index).start_address[i]);
                        printf("granularity[%d]: %d.\n", i, placement_table.at(placement_table_index).granularity[i]);
                    }
                    assert(0);
                }
            }
        }
        else if (remapping_request.sm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
            // this remapping_request moves block 0's data into fast memory
            START_ADDRESS_WIDTH start_address_in_fm = (remapping_request.address_in_fm >> DATA_LINE_OFFSET_BITS) % (START_ADDRESS_WIDTH(StartAddress::Max));
            START_ADDRESS_WIDTH start_address = (remapping_request.address_in_sm >> DATA_LINE_OFFSET_BITS) % (START_ADDRESS_WIDTH(StartAddress::Max));

            bool find_occupied_group = false;
            REMAPPING_LOCATION_WIDTH occupied_group_number = 0;
            MIGRATION_GRANULARITY_WIDTH used_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None);
            for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
            {
                used_space += placement_table.at(placement_table_index).granularity[i];
                START_ADDRESS_WIDTH accumulated_group_end_address = used_space - 1;

                // find the group in correct position
                if (start_address_in_fm <= accumulated_group_end_address)
                {
                    if (placement_table.at(placement_table_index).tag[i] == remapping_request.fm_location)
                    {
                        // sanity check
                        if (placement_table.at(placement_table_index).granularity[i] != remapping_request.size)
                        {
                            std::cout << __func__ << ": migration granularity calculation error." << std::endl;
                            assert(0);
                        }

                        find_occupied_group = true;
                        occupied_group_number = i;
                        break;
                    }
                }
            }

            if (find_occupied_group == false)
            {
                std::cout << __func__ << ": occupied_group_number calculation error." << std::endl;
                for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
                {
                    printf("tag[%d]: %d, ", i, placement_table.at(placement_table_index).tag[i]);
                    printf("start_address[%d]: %d, ", i, placement_table.at(placement_table_index).start_address[i]);
                    printf("granularity[%d]: %d.\n", i, placement_table.at(placement_table_index).granularity[i]);
                }
                printf("\nfm_location: %d, ", remapping_request.fm_location);
                printf("sm_location: %d, ", remapping_request.sm_location);
                printf("size: %d.\n", remapping_request.size);
                printf("start_address_in_fm: %d, ", start_address_in_fm);
                printf("start_address: %d.\n", start_address);
                assert(0);
            }

            // fill tag in placement table entry
            placement_table.at(placement_table_index).tag[occupied_group_number] = remapping_request.sm_location;

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
                for (REMAPPING_LOCATION_WIDTH_SIGN i = occupied_group_number - 1; i >= 0; i--) // go forward to check and mark invalid groups
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

            // calculate the free space in fast memory for this set
            int16_t free_space = MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::KiB_4);
            for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
            {
                // traverse the placement entry to calculate the free space in fast memory.
                free_space -= placement_table.at(placement_table_index).granularity[i];
            }

            // sanity check
            if (free_space < MIGRATION_GRANULARITY_WIDTH(MigrationGranularity::None))
            {
                std::cout << __func__ << ": free_space calculation error 2." << std::endl;
                for (REMAPPING_LOCATION_WIDTH i = 0; i < placement_table.at(placement_table_index).cursor; i++)
                {
                    printf("tag[%d]: %d, ", i, placement_table.at(placement_table_index).tag[i]);
                    printf("start_address[%d]: %d, ", i, placement_table.at(placement_table_index).start_address[i]);
                    printf("granularity[%d]: %d.\n", i, placement_table.at(placement_table_index).granularity[i]);
                }
                assert(0);
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
#if (IMMEDIATE_EVICTION == ENABLE)
#else
        // Overhead here is heavy. OpenMP is necessary.
#if (USE_OPENMP == ENABLE)
#pragma omp parallel
        {
#pragma omp for
#endif  // USE_OPENMP
            for (uint64_t i = 0; i < total_capacity_at_data_block_granularity; i++)
            {
                counter_table[i] >>= 1; // halve the counter value
                if (counter_table[i] == 0)
                {
                    hotness_table.at(i) = false;    // mark cold data block
                    for (START_ADDRESS_WIDTH j = START_ADDRESS_WIDTH(StartAddress::Zero); j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
                    {
                        // clear accessed data line
                        access_table.at(i).access[j] = false;
                    }
                }

#if (STATISTICS_INFORMATION == ENABLE)
                // count the estimated spatial locality
                START_ADDRESS_WIDTH count_access = START_ADDRESS_WIDTH(StartAddress::Zero);
                for (START_ADDRESS_WIDTH j = START_ADDRESS_WIDTH(StartAddress::Zero); j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
                {
                    if (access_table.at(i).temporal_access_stats[j] == true)
                    {
                        count_access++;
                        access_table.at(i).temporal_access_stats[j] = false;    // clear this bit
                    }
                }

                if (access_table.at(i).estimated_spatial_locality_stats < count_access)
                {
                    // store the biggest estimated spatial locality
                    access_table.at(i).estimated_spatial_locality_stats = count_access;
                }
#endif  // STATISTICS_INFORMATION
            }
#if (USE_OPENMP == ENABLE)
        }
#endif  // USE_OPENMP
#endif  // IMMEDIATE_EVICTION
    }

    cycle++;
}

#if (COLD_DATA_DETECTION_IN_GROUP == ENABLE)
void OS_TRANSPARENT_MANAGEMENT::cold_data_detection_in_group(uint64_t source_address)
{
    uint64_t data_block_address = source_address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in placement table
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the tag of the data block address

    for (REMAPPING_LOCATION_WIDTH i = 0; i < expected_number_in_congruence_group; i++)
    {
        if (i != tag)
        {
            REMAPPING_LOCATION_WIDTH location = i;
            uint64_t data_base_address_to_evict = replace_bits(base_remapping_address, uint64_t(location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
            uint64_t data_block_address_to_evict = data_base_address_to_evict >> DATA_MANAGEMENT_OFFSET_BITS;

            counter_table[data_block_address_to_evict] >>= 1; // halve the counter value
            if (counter_table[data_block_address_to_evict] == 0)
            {
                hotness_table.at(data_block_address_to_evict) = false;    // mark cold data block
                for (START_ADDRESS_WIDTH j = 0; j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
                {
                    // clear accessed data line
                    access_table.at(data_block_address_to_evict).access[j] = false;
                }
            }
        }
    }
}
#endif  // COLD_DATA_DETECTION_IN_GROUP

bool OS_TRANSPARENT_MANAGEMENT::cold_data_eviction(uint64_t source_address, float queue_busy_degree)
{
#if (DATA_EVICTION == ENABLE)
    uint64_t data_block_address = source_address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in placement table
    uint64_t base_remapping_address = placement_table_index << DATA_MANAGEMENT_OFFSET_BITS;
    REMAPPING_LOCATION_WIDTH tag = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the tag of the data block address
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
            // don't evict the data block which has same tag.
            continue;
        }
        else if (placement_table.at(placement_table_index).tag[i] != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
        {
#if (IMMEDIATE_EVICTION == ENABLE)
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[i];
            uint64_t data_base_address_to_evict = replace_bits(base_remapping_address, uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
            uint64_t data_block_address_to_evict = data_base_address_to_evict >> DATA_MANAGEMENT_OFFSET_BITS;
            for (START_ADDRESS_WIDTH j = 0; j < START_ADDRESS_WIDTH(StartAddress::Max); j++)
            {
                // clear accessed data line
                access_table.at(data_block_address_to_evict).access[j] = false;
            }

            is_cold = true;
            occupied_group_number = i;
            used_space -= placement_table.at(placement_table_index).granularity[occupied_group_number];
            break;
#else
            // check whether this data block is cold
            REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[i];
            uint64_t data_base_address_to_evict = replace_bits(base_remapping_address, uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
            uint64_t data_block_address_to_evict = data_base_address_to_evict >> DATA_MANAGEMENT_OFFSET_BITS;

            if (hotness_table.at(data_block_address_to_evict) == false) // this data block is cold
            {
                is_cold = true;
                occupied_group_number = i;
                used_space -= placement_table.at(placement_table_index).granularity[occupied_group_number];
                break;
            }
#endif  // IMMEDIATE_EVICTION
        }
    }

    if (is_cold)
    {
        for (REMAPPING_LOCATION_WIDTH i = occupied_group_number; i < placement_table.at(placement_table_index).cursor; i++)
        {
            // check whether this tag is the tag of cold data block
            if (placement_table.at(placement_table_index).tag[i] == placement_table.at(placement_table_index).tag[occupied_group_number])
            {
                REMAPPING_LOCATION_WIDTH sm_location = placement_table.at(placement_table_index).tag[i];

                // follow rule 2 (data blocks belonging to NM are recovered to the original locations)
                START_ADDRESS_WIDTH start_address_in_fm = used_space;
                START_ADDRESS_WIDTH start_address = placement_table.at(placement_table_index).start_address[i];

                tag = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero);

                // prepare a remapping request
                RemappingRequest remapping_request;
                remapping_request.address_in_fm = replace_bits(base_remapping_address + (start_address_in_fm << DATA_LINE_OFFSET_BITS), uint64_t(tag) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);
                remapping_request.address_in_sm = replace_bits(base_remapping_address + (start_address << DATA_LINE_OFFSET_BITS), uint64_t(sm_location) << fast_memory_offset_bit, set_msb, fast_memory_offset_bit);

                // indicate where the data come from for address_in_fm and address_in_sm.
                remapping_request.fm_location = sm_location;    // this shouldn't be RemappingLocation::Zero.
                remapping_request.sm_location = tag;            // this should be RemappingLocation::Zero.

                remapping_request.size = placement_table.at(placement_table_index).granularity[i];

                bool enqueue = false;
                if (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD)
                {
                    enqueue = enqueue_remapping_request(remapping_request);
                }

                if (enqueue)
                {
                    // new eviction request is issued.
                    outputchampsimstatistics.data_eviction_success++;
                }
                else
                {
                    // no eviction request is issued.
                    outputchampsimstatistics.data_eviction_failure++;
                }
            }

            // prepare address calculation for next checking
            used_space += placement_table.at(placement_table_index).granularity[i];
        }
    }

#endif  // DATA_EVICTION
    return true;
}

bool OS_TRANSPARENT_MANAGEMENT::enqueue_remapping_request(RemappingRequest& remapping_request)
{
    uint64_t data_block_address = remapping_request.address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t placement_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;

    // check duplicated remapping request in remapping_request_queue
    // if duplicated remapping requests exist, we won't add this new remapping request into the remapping_request_queue.
    bool duplicated_remapping_request = false;
    for (uint64_t i = 0; i < remapping_request_queue.size(); i++)
    {
        uint64_t data_block_address_to_check = remapping_request_queue[i].address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t placement_table_index_to_check = data_block_address_to_check % fast_memory_capacity_at_data_block_granularity;

        if (placement_table_index_to_check == placement_table_index)
        {
            duplicated_remapping_request = true;    // find a duplicated remapping request

            // check whether the remapping_request moves block 0's data into fast memory
            if ((remapping_request.fm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)) && (remapping_request_queue[i].fm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
            {
                // for the request that moves block 0's data into slow memory, only one remapping request for the same set can exist in remapping_request_queue to maintain data consistency
                if ((remapping_request_queue[i].address_in_fm == remapping_request.address_in_fm) && ((remapping_request_queue[i].address_in_sm == remapping_request.address_in_sm)))
                {
                    if (remapping_request.size > remapping_request_queue[i].size)
                    {
                        remapping_request_queue[i].size = remapping_request.size;   // update size for data swapping
                    }

                    // new remapping request won't be issued, but the duplicated one is updated.
                    return true;
                }
            }
            else if ((remapping_request.sm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)) && (remapping_request_queue[i].sm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
            {
                // for the request that moves block 0's data into fast memory, multiple remapping requests for the same set can exist as long as the data movement in fast memory are different
                if ((remapping_request_queue[i].address_in_fm == remapping_request.address_in_fm) && (remapping_request_queue[i].address_in_sm == remapping_request.address_in_sm))
                {
                    if (remapping_request.size > remapping_request_queue[i].size)
                    {
                        remapping_request_queue[i].size = remapping_request.size;   // update size for data swapping
                    }

                    // new remapping request won't be issued, but the duplicated one is updated.
                    return true;
                }
                else if (remapping_request_queue[i].address_in_fm != remapping_request.address_in_fm)
                {
                    duplicated_remapping_request = false;
                    continue;
                }
            }

            break;
        }
    }

    // add new remapping request to queue
    if (duplicated_remapping_request == false)
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
    // new remapping request is issued.
    return true;
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

START_ADDRESS_WIDTH OS_TRANSPARENT_MANAGEMENT::adjust_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity)
{
    START_ADDRESS_WIDTH updated_end_address = end_address;

    // check whether this migration granularity is beyond the block's range
    while (true)
    {
        if ((start_address + migration_granularity - 1) >= START_ADDRESS_WIDTH(StartAddress::Max))
        {
#if (FLEXIBLE_GRANULARITY == ENABLE)
            migration_granularity = end_address - start_address + 1;
#else
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
#endif  // FLEXIBLE_GRANULARITY
        }
        else
        {
            // this migration granularity is within the block's range.
            updated_end_address = start_address + migration_granularity - 1;
            break;
        }
    }

    return updated_end_address;
}

START_ADDRESS_WIDTH OS_TRANSPARENT_MANAGEMENT::round_down_migration_granularity(const START_ADDRESS_WIDTH start_address, const START_ADDRESS_WIDTH end_address, MIGRATION_GRANULARITY_WIDTH& migration_granularity)
{
    START_ADDRESS_WIDTH updated_end_address = end_address;

    // check whether this migration granularity is beyond the block's end address
    while (true)
    {
        if ((start_address + migration_granularity - 1) > end_address)
        {
#if (FLEXIBLE_GRANULARITY == ENABLE)
            migration_granularity = end_address - start_address + 1;
#else
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
#endif  // FLEXIBLE_GRANULARITY
        }
        else
        {
            // this migration granularity is within the block's end address.
            updated_end_address = start_address + migration_granularity - 1;
            break;
        }
    }

    return updated_end_address;
}

#endif  // IDEAL_VARIABLE_GRANULARITY
#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

#include "ideal_single_mempod.h"

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#if (IDEAL_SINGLE_MEMPOD == ENABLE)

// complete
OS_TRANSPARENT_MANAGEMENT::OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address)
    : total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
    total_capacity_at_granularity(max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_capacity_at_granularity(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_offset_bit(DATA_MANAGEMENT_OFFSET_BITS),
    mea_counter_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, MEA_COUNTER_WIDTH>())),
    address_remapping_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>())),
    invert_address_remapping_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>()))
{
    remapping_request_queue_congestion = 0;
    intervals = 1;

    interval_cycle = CPU_FREQUENCY * (double)TIME_INTERVAL_MEMPOD_us / MEMORY_CONTROLLER_CLOCK_SCALE;
    next_interval_cycle = interval_cycle;

    /* initializing address_remapping_table and invert_address_remapping_table */
    // TODO: I think there is faster way to construct mapping.

    for (REMAPPING_TABLE_ENTRY_WIDTH itr_map = 0; itr_map < fast_memory_capacity_at_granularity; itr_map++)
    {
        address_remapping_table[itr_map] = itr_map;
        invert_address_remapping_table[itr_map] = itr_map;
    }
    for (REMAPPING_TABLE_ENTRY_WIDTH itr_map = fast_memory_capacity_at_granularity; itr_map < total_capacity_at_granularity; itr_map++)
    {
        address_remapping_table[itr_map] = itr_map;
    }
    assert(address_remapping_table.size() == total_capacity_at_granularity);
    assert(invert_address_remapping_table.size() == fast_memory_capacity_at_granularity);
};

// complete
OS_TRANSPARENT_MANAGEMENT::~OS_TRANSPARENT_MANAGEMENT()
{
    outputchampsimstatistics.remapping_request_queue_congestion = remapping_request_queue_congestion;

    delete& mea_counter_table;
    delete& address_remapping_table;
    delete& invert_address_remapping_table;
};

#if (TRACKING_LOAD_STORE_STATISTICS == ENABLE)
// complete
bool OS_TRANSPARENT_MANAGEMENT::memory_activity_tracking(uint64_t address, uint8_t type, uint8_t type_origin, float queue_busy_degree)
{
#if (TRACKING_LOAD_ONLY)
    if (type_origin == RFO || type_origin == WRITEBACK) // CPU Store Instruction and LLC Writeback is ignored
    {
        return true;
    }
#endif // TRACKING_LOAD_ONLY

#if (TRACKING_READ_ONLY)
    if (type == 2) // Memory Write is ignored
    {
        return true;
    }
#endif // TRACKING_READ_ONLY

    if (address >= total_capacity)
    {
        std::cout << __func__ << ": address input error." << std::endl;
        return false;
    }

    uint64_t data_segment_address = address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    update_mea_counter(data_segment_address);
    return true;
};
#else
bool OS_TRANSPARENT_MANAGEMENT::memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree)
{
    if (address >= total_capacity)
    {
        std::cout << __func__ << ": address input error." << std::endl;
        return false;
    }

    uint64_t data_segment_address = address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    update_mea_counter(data_segment_address);
    return true;
};
#endif


// debugged
void OS_TRANSPARENT_MANAGEMENT::update_mea_counter(uint64_t segment_address)
{
    std::deque<REMAPPING_TABLE_ENTRY_WIDTH> data_to_delete;

    if (mea_counter_table.count(segment_address) == 1) // exist
    {
        if (mea_counter_table[segment_address] < (MEA_COUNTER_MAX_VALUE + 1u))
        {
            mea_counter_table[segment_address]++;
        }
    }
    else if (mea_counter_table.count(segment_address) == 0) // not exist
    {
        if (mea_counter_table.size() >= NUMBER_MEA_COUNTER) // MEA Counter is full
        {
            for (auto mea_itr = mea_counter_table.begin(); mea_itr != mea_counter_table.end(); ++mea_itr)
            {
                mea_itr->second--;
                if (mea_itr->second == 0)
                {   
                    data_to_delete.push_back(mea_itr->first);
                }
            }
            while (!data_to_delete.empty())
            {
                mea_counter_table.erase(data_to_delete.front());
                data_to_delete.pop_front();
            }
        }
        else // MEA Counter is not full
        {
           mea_counter_table[segment_address] = 1; 
        }
    }
    else
    {
        std::cout << __func__ << ": mea counter error" << std::endl;
        assert(0);
    }
};

// complete
void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(PACKET& packet)
{
    uint64_t data_segment_address = packet.address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t data_segment_offset = packet.address - (data_segment_address << DATA_MANAGEMENT_OFFSET_BITS);
#if (DEBUG_PRINTF == ENABLE)
    printf("physical_to_hardware_address(PACKET), p_segment %lu, h_segment %lu \n", data_segment_address, address_remapping_table[data_segment_address]);
#endif
    packet.h_address = (address_remapping_table[data_segment_address] << DATA_MANAGEMENT_OFFSET_BITS) + data_segment_offset;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(uint64_t& address)
{
    uint64_t data_segment_address = address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t data_segment_offset = address - (data_segment_address << DATA_MANAGEMENT_OFFSET_BITS);
#if (DEBUG_PRINTF == ENABLE)
    printf("physical_to_hardware_address(uint64_t), p_segment %lu, h_segment %lu \n", data_segment_address, address_remapping_table[data_segment_address]);
#endif
    address = (address_remapping_table[data_segment_address] << DATA_MANAGEMENT_OFFSET_BITS) + data_segment_offset;
};

// complete
bool OS_TRANSPARENT_MANAGEMENT::issue_remapping_request(RemappingRequest& remapping_request)
{
    if (remapping_request_queue.empty() == false)
    {
        remapping_request = remapping_request_queue.front();
        return true;
    }

    return false;
};

// complete
bool OS_TRANSPARENT_MANAGEMENT::finish_remapping_request()
{
    if (remapping_request_queue.empty() == false)
    {   
        RemappingRequest remapping_request = remapping_request_queue.front();
        remapping_request_queue.pop_front();

        /* update address_remapping_table */
        uint64_t data_segment_p_address_fm = remapping_request.p_address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t data_segment_p_address_sm = remapping_request.p_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;

        uint64_t data_segment_h_address_fm = remapping_request.h_address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t data_segment_h_address_sm = remapping_request.h_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;

        // sanity check
        if (data_segment_h_address_fm == data_segment_h_address_sm)
        {
            std::cout << __func__ << ": read remapping location error." << std::endl;
            abort();
        }

        address_remapping_table[data_segment_p_address_sm] = data_segment_h_address_fm;
        address_remapping_table[data_segment_p_address_fm] = data_segment_h_address_sm;

        /* update invert_address_remapping_table*/
        invert_address_remapping_table[data_segment_h_address_fm] = data_segment_p_address_sm;
    }
    else
    {
        std::cout << __func__ << ": remapping error." << std::endl;
        assert(0);
        return false;   // error
    }
    return true;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::cold_data_detection()
{
    cycle++;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::check_interval_swap(uint8_t swapping_states)
{   
    if (cycle >= next_interval_cycle)
    {   
        /* cancel remapping request what is not started in last epoch */
        cancel_not_started_remapping_request(swapping_states);

        /* get hot pages and victim pages */
        std::vector<REMAPPING_TABLE_ENTRY_WIDTH> hot_pages(mea_counter_table.size());
        get_hot_page_from_mea_counter(hot_pages);
        determine_swap_pair(hot_pages);

        /* set next interval */
        next_interval_cycle += interval_cycle;

        if (all_warmup_complete > NUM_CPUS)
        {
            intervals++;
        }

#if(MEA_COUNTER_RESET_EVERY_EPOCH)
        reset_mea_counter();
#endif // MEA_COUNTER_RESET_EVERY_EPOCH

    }
};

// complete
bool OS_TRANSPARENT_MANAGEMENT::enqueue_remapping_request(RemappingRequest& remapping_request)
{
/*
    uint64_t data_segment_address = remapping_request.h_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;

    // check duplicated remapping request in remapping_request_queue
    // if duplicated remapping requests exist, we won't add this new remapping request into the remapping_request_queue.
    bool duplicated_remapping_request = false;
    for (uint64_t i = 1; i < remapping_request_queue.size(); i++)
    {
        uint64_t data_block_address_to_check = remapping_request_queue[i].address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t line_location_table_index_to_check = data_block_address_to_check % fast_memory_capacity_at_data_block_granularity;

        if (line_location_table_index_to_check == line_location_table_index)
        {
            duplicated_remapping_request = true;    // find a duplicated remapping request

            break;
        }
    }

    if (duplicated_remapping_request == false)
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
    else
    {
        return false;
    }
*/
    if (remapping_request_queue.size() < REMAPPING_REQUEST_QUEUE_LENGTH)
    {
        if (remapping_request.h_address_in_fm == remapping_request.h_address_in_sm)    // check
        {
            std::cout << __func__ << ": add new remapping request error 2." << std::endl;
            abort();
        }

        // enqueue a remapping request
        remapping_request_queue.push_back(remapping_request);
    }
    else
    {   
        if (all_warmup_complete > NUM_CPUS)
        {
            remapping_request_queue_congestion++;
        }
    }

    return true;
}

// debugged
void OS_TRANSPARENT_MANAGEMENT::get_hot_page_from_mea_counter(std::vector<REMAPPING_TABLE_ENTRY_WIDTH>& hot_pages)
{   
    int hot_page_itr = 0;
    for (auto mea_itr = mea_counter_table.begin(); mea_itr != mea_counter_table.end(); ++mea_itr)
    {
        hot_pages[hot_page_itr] = mea_itr->first;
        hot_page_itr++;
    }
    std::sort(hot_pages.begin(),hot_pages.end());
};

// complete
void OS_TRANSPARENT_MANAGEMENT::reset_mea_counter()
{   
    mea_counter_table.clear();
};

// debugged
void OS_TRANSPARENT_MANAGEMENT::determine_swap_pair(std::vector<REMAPPING_TABLE_ENTRY_WIDTH>& hot_pages)
{

#if (PRINT_SWAPS_PER_EPOCH_MEMPOD == ENABLE)
    uint32_t swaps_per_epoch = 0;
#endif // PRINT_SWAPS_PER_EPOCH_MEMPOD

    REMAPPING_TABLE_ENTRY_WIDTH hot_page_h_address;
    REMAPPING_TABLE_ENTRY_WIDTH hot_page_p_address;
    std::vector<REMAPPING_TABLE_ENTRY_WIDTH> hot_page_in_fm;
    std::vector<PhysicalHardwareAddressTuple> hot_page_in_sm;
    for (uint32_t hot_page_itr = 0; hot_page_itr < hot_pages.size(); hot_page_itr++)
    {   
        hot_page_p_address = hot_pages[hot_page_itr];
        hot_page_h_address = address_remapping_table[hot_page_p_address];
        PhysicalHardwareAddressTuple hot_page_ph_address;
        hot_page_ph_address.h_address = hot_page_h_address;
        hot_page_ph_address.p_address = hot_page_p_address;
        if (hot_page_h_address < fast_memory_capacity_at_granularity) // if hot_page in Fast Memory
        {
            hot_page_in_fm.push_back(hot_page_ph_address.h_address);
        }
        else if (hot_page_h_address < total_capacity_at_granularity) // if hot_page in Slow Memory
        {  
            hot_page_in_sm.push_back(hot_page_ph_address);
        }
        else
        {
            std::cout << __func__ << ": hot page range error" << std::endl;
            assert(0);            
        }
    }

    std::sort(hot_page_in_fm.begin(),hot_page_in_fm.end());
    std::sort(hot_page_in_sm.begin(),hot_page_in_sm.end());

    for (uint32_t hot_page_in_sm_itr = 0; hot_page_in_sm_itr < hot_page_in_sm.size(); hot_page_in_sm_itr++)
    {   
        while (std::find(hot_page_in_fm.begin(), hot_page_in_fm.end(), swap_fm_address_itr) != hot_page_in_fm.end())
        {
            swap_fm_address_itr++;
            //printf("swap_fm_address_itr incremented \n");
        }
        
        RemappingRequest remapping_request;
        remapping_request.p_address_in_fm = invert_address_remapping_table[swap_fm_address_itr] << DATA_MANAGEMENT_OFFSET_BITS;
        remapping_request.p_address_in_sm = hot_page_in_sm[hot_page_in_sm_itr].p_address << DATA_MANAGEMENT_OFFSET_BITS;
        remapping_request.h_address_in_fm = swap_fm_address_itr << DATA_MANAGEMENT_OFFSET_BITS;
        remapping_request.h_address_in_sm = hot_page_in_sm[hot_page_in_sm_itr].h_address << DATA_MANAGEMENT_OFFSET_BITS;
        remapping_request.size = SWAP_DATA_CACHE_LINES;
        enqueue_remapping_request(remapping_request);

#if (PRINT_SWAPS_PER_EPOCH_MEMPOD == ENABLE)
        swaps_per_epoch++;
#endif // PRINT_SWAPS_PER_EPOCH_MEMPOD

        swap_fm_address_itr++;

        // if this iterator's value is over fast memory range, then swap_fm_address_itr = 0. (loop)
        swap_fm_address_itr = swap_fm_address_itr % fast_memory_capacity_at_granularity;

    }

#if (PRINT_SWAPS_PER_EPOCH_MEMPOD == ENABLE)
        if (all_warmup_complete > NUM_CPUS)
        {
            printf("[SWAPS_PER_EPOCH] interval: %u swaps: %u \n",intervals,swaps_per_epoch);
        }
#endif // PRINT_SWAPS_PER_EPOCH_MEMPOD
};

// complete
void OS_TRANSPARENT_MANAGEMENT::cancel_not_started_remapping_request(uint8_t swapping_states)
{
    switch (swapping_states)
    {
        case 0: // the swapping unit is idle
            {   
                remapping_request_queue.clear();
                break;
            }
        case 1: // the swapping unit is busy
            {  
                RemappingRequest remapping_request_in_progress = remapping_request_queue.front();
                remapping_request_queue.clear();
                remapping_request_queue.push_back(remapping_request_in_progress);
                break;
            }
        case 2: // the swapping unit finishes a swapping request
            {   
                remapping_request_queue.clear();
                break;
            }
        default:
            break;
    }
};


#endif  // IDEAL_SINGLE_MEMPOD
#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT

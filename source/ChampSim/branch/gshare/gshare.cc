#include "ChampSim/branch/gshare/gshare.h"

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

std::size_t gshare::gs_table_hash(champsim::address ip, std::bitset<GLOBAL_HISTORY_LENGTH> bh_vector)
{
    constexpr champsim::data::bits LOG2_HISTORY_TABLE_SIZE {champsim::lg2(GS_HISTORY_TABLE_SIZE)};
    constexpr champsim::data::bits LENGTH {GLOBAL_HISTORY_LENGTH};

    std::size_t hash = bh_vector.to_ullong();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE, champsim::data::bits {}>().to<std::size_t>();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE + LENGTH, LENGTH>().to<std::size_t>();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE + 2 * LENGTH, 2 * LENGTH>().to<std::size_t>();

    return hash % GS_HISTORY_TABLE_SIZE;
}

bool gshare::predict_branch(champsim::address ip)
{
    auto gs_hash = gs_table_hash(ip, branch_history_vector);
    auto value   = gs_history_table[gs_hash];
    return value.value() >= (value.maximum / 2);
}

void gshare::last_branch_result(champsim::address ip, champsim::address branch_target, bool taken, uint8_t branch_type)
{
    auto gs_hash = gs_table_hash(ip, branch_history_vector);
    gs_history_table[gs_hash] += taken ? 1 : -1;

    // update branch history vector
    branch_history_vector <<= 1;
    branch_history_vector[0] = taken;
}

// void O3_CPU::bpred_branchDgshare_initialize_branch_predictor()
// {
// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//     fprintf(output_statistics.file_handler, "CPU %d Gshare branch predictor\n", cpu);
// #else
//     std::cout << "CPU " << cpu << " Gshare branch predictor" << std::endl;
// #endif
// }

// uint8_t O3_CPU::bpred_branchDgshare_predict_branch(uint64_t ip)
// {
//     auto gs_hash = ::gs_table_hash(ip, ::branch_history_vector[this]);
//     auto value   = ::gs_history_table[this][gs_hash];
//     return value.value() >= (value.maximum / 2);
// }

// void O3_CPU::bpred_branchDgshare_last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type)
// {
//     auto gs_hash = gs_table_hash(ip, ::branch_history_vector[this]);
//     ::gs_history_table[this][gs_hash] += taken ? 1 : -1;

//     // update branch history vector
//     ::branch_history_vector[this] <<= 1;
//     ::branch_history_vector[this][0] = taken;
// }

#else

std::size_t gshare::gs_table_hash(champsim::address ip, std::bitset<GLOBAL_HISTORY_LENGTH> bh_vector)
{
    constexpr champsim::data::bits LOG2_HISTORY_TABLE_SIZE {champsim::lg2(GS_HISTORY_TABLE_SIZE)};
    constexpr champsim::data::bits LENGTH {GLOBAL_HISTORY_LENGTH};

    std::size_t hash = bh_vector.to_ullong();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE, champsim::data::bits {}>().to<std::size_t>();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE + LENGTH, LENGTH>().to<std::size_t>();
    hash ^= ip.slice<LOG2_HISTORY_TABLE_SIZE + 2 * LENGTH, 2 * LENGTH>().to<std::size_t>();

    return hash % GS_HISTORY_TABLE_SIZE;
}

bool gshare::predict_branch(champsim::address ip)
{
    auto gs_hash = gs_table_hash(ip, branch_history_vector);
    auto value   = gs_history_table[gs_hash];
    return value.value() >= (value.maximum / 2);
}

void gshare::last_branch_result(champsim::address ip, champsim::address branch_target, bool taken, uint8_t branch_type)
{
    auto gs_hash = gs_table_hash(ip, branch_history_vector);
    gs_history_table[gs_hash] += taken ? 1 : -1;

    // update branch history vector
    branch_history_vector <<= 1;
    branch_history_vector[0] = taken;
}

#endif // USER_CODES

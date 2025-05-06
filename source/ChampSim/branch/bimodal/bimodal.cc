#include "ChampSim/branch/bimodal/bimodal.h"

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)

bool bimodal::predict_branch(champsim::address ip)
{
    auto value = bimodal_table[hash(ip)];
    return value.value() > (value.maximum / 2);
}

void bimodal::last_branch_result(champsim::address ip, champsim::address branch_target, bool taken, uint8_t branch_type)
{
    bimodal_table[hash(ip)] += taken ? 1 : -1;
}

// void O3_CPU::bpred_branchDbimodal_initialize_branch_predictor()
// {
// #if (PRINT_STATISTICS_INTO_FILE == ENABLE)
//     fprintf(output_statistics.file_handler, "CPU %d Bimodal branch predictor\n", cpu);
// #else
//     std::cout << "CPU " << cpu << " Bimodal branch predictor" << std::endl;
// #endif
// }

// uint8_t O3_CPU::bpred_branchDbimodal_predict_branch(uint64_t ip)
// {
//     auto value = bimodal_table[hash(ip)];
//     return value.value() > (value.maximum / 2);
// }

// void O3_CPU::bpred_branchDbimodal_last_branch_result(uint64_t ip, uint64_t branch_target, uint8_t taken, uint8_t branch_type)
// {
//     bimodal_table[hash(ip)] += taken ? 1 : -1;
// }

#else

bool bimodal::predict_branch(champsim::address ip)
{
    auto value = bimodal_table[hash(ip)];
    return value.value() > (value.maximum / 2);
}

void bimodal::last_branch_result(champsim::address ip, champsim::address branch_target, bool taken, uint8_t branch_type)
{
    bimodal_table[hash(ip)] += taken ? 1 : -1;
}

#endif // USER_CODES

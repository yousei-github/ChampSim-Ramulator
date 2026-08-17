#ifndef TEST_INSTR_H
#define TEST_INSTR_H

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/instruction.h"
#else
#include "instruction.h"
#endif /* USER_CODES */

namespace champsim::test
{
ooo_model_instr instruction_with_ip(champsim::address ip);
ooo_model_instr instruction_with_ip(uint64_t ip);
ooo_model_instr branch_instruction_with_ip(champsim::address ip);
ooo_model_instr branch_instruction_with_ip(uint64_t ip);
ooo_model_instr instruction_with_registers(uint8_t reg);
ooo_model_instr instruction_with_ip_and_source_memory(champsim::address ip, champsim::address smem);
} // namespace champsim::test

#endif

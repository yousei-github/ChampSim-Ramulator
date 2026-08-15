#ifndef TEST_REPL_INTERFACE_H
#define TEST_REPL_INTERFACE_H

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/channel.h"
#else
#include "channel.h"
#endif /* USER_CODES */

namespace test
{
struct repl_update_interface {
  uint32_t cpu;
  long set;
  long way;
  champsim::address full_addr;
  champsim::address ip;
  champsim::address victim_addr;
  access_type type;
  bool hit;
};

struct repl_fill_interface {
  uint32_t cpu;
  long set;
  long way;
  champsim::address full_addr;
  champsim::address ip;
  champsim::address victim_addr;
  access_type type;
};
} // namespace test

#endif

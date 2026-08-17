#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/tracereader.h"
#else
#include "tracereader.h"
#endif /* USER_CODES */

TEST_CASE("A tracereader can be constructed from a type without an eof() member function")
{
  champsim::tracereader uut{[]() {
    return ooo_model_instr{0, input_instr{}};
  }};
  REQUIRE_FALSE(uut.eof());
  (void)uut();
  REQUIRE_FALSE(uut.eof());
}

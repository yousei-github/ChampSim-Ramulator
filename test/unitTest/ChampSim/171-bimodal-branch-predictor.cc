#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/branch/bimodal/bimodal.h"
#else
#include "../../../branch/bimodal/bimodal.h"
#endif /* USER_CODES */

TEST_CASE("The bimodal predictor predicts taken after many taken branches")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  }

  REQUIRE(uut.predict_branch(ip_under_test));
}

TEST_CASE("The bimodal predictor predicts not taken after many not-taken branches")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  }

  REQUIRE_FALSE(uut.predict_branch(ip_under_test));
}

TEST_CASE("After saturating not-taken, the bimodal predictor continues to predict not taken after one taken branch")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  }

  uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  REQUIRE_FALSE(uut.predict_branch(ip_under_test));
}

TEST_CASE("After saturating not-taken, the bimodal predictor predicts taken after two taken branches")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  }

  uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  REQUIRE(uut.predict_branch(ip_under_test));
}

TEST_CASE("After saturating taken, the bimodal predictor continues to predict taken after one not-taken branch")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  }

  uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  REQUIRE(uut.predict_branch(ip_under_test));
}

TEST_CASE("After saturating taken, the bimodal predictor predicts not taken after two not-taken branches")
{
  bimodal uut{nullptr};
  champsim::address ip_under_test{0xdeadbeef};

  for (std::size_t i{0}; i < 100; ++i) {
    uut.last_branch_result(ip_under_test, champsim::address{}, true, 0);
  }

  uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  uut.last_branch_result(ip_under_test, champsim::address{}, false, 0);
  REQUIRE_FALSE(uut.predict_branch(ip_under_test));
}

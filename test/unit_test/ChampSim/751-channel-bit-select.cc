#include <catch.hpp>
#include <fmt/core.h>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/dram_controller.h"
#else
#include "dram_controller.h"
#endif /* USER_CODES */

/**
 * These tests exercise ChampSim's own DRAM model. Under RAMULATOR or RAMULATOR2 that model is
 * replaced by the Ramulator bridge, so MEMORY_CONTROLLER / DRAM_ADDRESS_MAPPING as used below do
 * not exist and the file compiles to no tests at all. Build with both Ramulator toggles set to
 * DISABLE in ProjectConfiguration.h to run them.
 */
#if (RAMULATOR != ENABLE) && (RAMULATOR2 != ENABLE)

TEST_CASE("A DRAM channel can identify the bits of a row")
{
  auto rows = GENERATE(as<std::size_t>{}, 2, 8);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  auto segment_to_find = (1ull << champsim::lg2(rows)) - 1;
  champsim::address addr{segment_to_find << (3 + champsim::lg2(columns) + champsim::lg2(ranks) + champsim::lg2(banks) + champsim::lg2(bankgroups))};

  INFO(fmt::format("address: {}", addr));
  REQUIRE(uut.get_row(addr) == segment_to_find);
}

TEST_CASE("A DRAM channel can identify the bits of a column")
{
  auto rows = GENERATE(as<std::size_t>{}, 2, 8);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  auto segment_to_find = (1ull << champsim::lg2(columns / prefetch_size)) - 1;
  champsim::address addr{segment_to_find << (3 + champsim::lg2(banks) + champsim::lg2(bankgroups) + champsim::lg2(prefetch_size))};

  INFO(fmt::format("address: {}", addr));
  REQUIRE(uut.get_column(addr) == segment_to_find);
}

TEST_CASE("A DRAM channel can identify the bits of a rank")
{
  auto rows = GENERATE(as<std::size_t>{}, 2, 8);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  auto segment_to_find = (1ull << champsim::lg2(ranks)) - 1;
  champsim::address addr{segment_to_find << (3 + champsim::lg2(columns) + champsim::lg2(banks) + champsim::lg2(bankgroups))};

  INFO(fmt::format("address: {}", addr));
  REQUIRE(uut.get_rank(addr) == segment_to_find);
}

TEST_CASE("A DRAM channel can identify the bits of a bank")
{
  auto rows = GENERATE(as<std::size_t>{}, 2, 8);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  auto segment_to_find = (1ull << champsim::lg2(banks)) - 1;
  champsim::address addr{segment_to_find << (3 + champsim::lg2(prefetch_size) + champsim::lg2(bankgroups))};

  INFO(fmt::format("address: {}", addr));
  REQUIRE(uut.get_bank(addr) == segment_to_find);
}

TEST_CASE("A DRAM channel can identify the bits of a bankgroup")
{
  auto rows = GENERATE(as<std::size_t>{}, 2, 8);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  auto segment_to_find = (1ull << champsim::lg2(bankgroups)) - 1;
  champsim::address addr{segment_to_find << (3 + champsim::lg2(prefetch_size))};

  INFO(fmt::format("address: {}", addr));
  REQUIRE(uut.get_bankgroup(addr) == segment_to_find);
}

TEST_CASE("A permutation of channels is provided per row")
{
  auto row = GENERATE(as<unsigned long>{}, 1, 3, 7, 15, 23, 117, 257, 1023, 1635, 2778, 4092, 8423, 15266, 45555, 65432);
  auto channels = GENERATE(as<std::size_t>{}, 1, 2, 4);
  auto rows = GENERATE(as<std::size_t>{}, 65536, 131072);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, channels, bankgroups, banks, columns, ranks, rows);

  // set row address
  champsim::address row_addr{
      row << (3 + champsim::lg2(columns) + champsim::lg2(ranks) + champsim::lg2(banks) + champsim::lg2(bankgroups) + champsim::lg2(channels))};

  std::vector<unsigned long> decoded_channels(channels, 0);

  for (unsigned int c = 0; c < channels; c++) {
    // set bank_address
    champsim::address addr = champsim::address{row_addr.to<unsigned long>() + (c << (3 + champsim::lg2(prefetch_size)))};
    decoded_channels[uut.get_channel(addr)]++;
  }
  REQUIRE_THAT(decoded_channels, Catch::Matchers::Equals(std::vector<unsigned long>(channels, 1)));
}

TEST_CASE("A permutation of banks is provided per row")
{
  auto row = GENERATE(as<unsigned long>{}, 1, 3, 7, 15, 23, 117, 257, 1023, 1635, 2778, 4092, 8423, 15266, 45555, 65432);
  auto rows = GENERATE(as<std::size_t>{}, 65536, 131072);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  // set row address
  champsim::address row_addr{row << (3 + champsim::lg2(columns) + champsim::lg2(ranks) + champsim::lg2(banks) + champsim::lg2(bankgroups))};

  std::vector<unsigned long> decoded_banks(banks, 0);

  for (unsigned int b = 0; b < banks; b++) {
    // set bank_address
    champsim::address addr = champsim::address{row_addr.to<unsigned long>() + (b << (3 + champsim::lg2(prefetch_size) + champsim::lg2(bankgroups)))};
    decoded_banks[uut.get_bank(addr)]++;
  }
  REQUIRE_THAT(decoded_banks, Catch::Matchers::Equals(std::vector<unsigned long>(banks, 1)));
}

TEST_CASE("A permutation of bankgroups is provided per row")
{
  auto row = GENERATE(as<unsigned long>{}, 1, 3, 7, 15, 23, 117, 257, 1023, 1635, 2778, 4092, 8423, 15266, 45555, 65432);
  auto rows = GENERATE(as<std::size_t>{}, 65536, 131072);
  auto columns = GENERATE(as<std::size_t>{}, 128, 256);
  auto ranks = GENERATE(as<std::size_t>{}, 2, 8);
  auto bankgroups = GENERATE(as<std::size_t>{}, 2, 8);
  auto banks = GENERATE(as<std::size_t>{}, 2, 8);
  auto prefetch_size = GENERATE(as<std::size_t>{}, 8, 16);
  auto uut = DRAM_ADDRESS_MAPPING(champsim::data::bytes{8}, prefetch_size, 1, bankgroups, banks, columns, ranks, rows);

  // set row address
  champsim::address row_addr{row << (3 + champsim::lg2(columns) + champsim::lg2(ranks) + champsim::lg2(banks) + champsim::lg2(bankgroups))};

  std::vector<unsigned long> decoded_bankgroups(bankgroups, 0);

  for (unsigned int b = 0; b < bankgroups; b++) {
    // set bank_address
    champsim::address addr = champsim::address{row_addr.to<unsigned long>() + (b << (3 + champsim::lg2(prefetch_size)))};
    decoded_bankgroups[uut.get_bankgroup(addr)]++;
  }
  REQUIRE_THAT(decoded_bankgroups, Catch::Matchers::Equals(std::vector<unsigned long>(bankgroups, 1)));
}

#endif /* RAMULATOR, RAMULATOR2 */

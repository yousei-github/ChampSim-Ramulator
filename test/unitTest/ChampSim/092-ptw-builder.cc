#include <array>
#include <catch.hpp>

#include "ProjectConfiguration.h" // User file

#if (USER_CODES == ENABLE)
#include "ChampSim/channel.h"
#include "ChampSim/dram_controller.h"
#include "ChampSim/ptw.h"
#include "ChampSim/vmem.h"
#else
#include "channel.h"
#include "dram_controller.h"
#include "ptw.h"
#include "vmem.h"
#endif /* USER_CODES */

TEST_CASE("The MSHR factor uses the number of upper levels to determine the PTW's default number of MSHRs")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = GENERATE(1u, 2u, 4u, 6u);
  auto mshr_factor = 2u;
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.mshr_factor((double)mshr_factor);

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  REQUIRE(uut.MSHR_SIZE == mshr_factor * num_uls);
}

TEST_CASE("The MSHR factor can control the PTW's default number of MSHRs")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = 2u;
  auto mshr_factor = GENERATE(1u, 2u, 4u, 6u);
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.mshr_factor((double)mshr_factor);

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  REQUIRE(uut.MSHR_SIZE == mshr_factor * num_uls);
}

TEST_CASE("Specifying the PTW's MSHR size overrides the MSHR factor")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = 2u;
  auto mshr_factor = 2u;
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.mshr_factor((double)mshr_factor);

  buildA.mshr_size(6);

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  REQUIRE(uut.MSHR_SIZE == 6);
}

TEST_CASE("The bandwidth factor uses the number of upper levels to determine the PTW's default tag and fill bandwidth")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = GENERATE(1u, 2u, 4u, 6u);
  auto bandwidth_factor = 2u;
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.bandwidth_factor((double)bandwidth_factor);

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  CHECK(uut.MAX_READ == champsim::bandwidth::maximum_type{bandwidth_factor * num_uls});
  CHECK(uut.MAX_FILL == champsim::bandwidth::maximum_type{bandwidth_factor * num_uls});
}

TEST_CASE("The bandwidth factor can control the PTW's default tag bandwidth")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = 2u;
  auto bandwidth_factor = GENERATE(1u, 2u, 4u, 6u);
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.bandwidth_factor((double)bandwidth_factor);

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  CHECK(uut.MAX_READ == champsim::bandwidth::maximum_type{bandwidth_factor * num_uls});
  CHECK(uut.MAX_FILL == champsim::bandwidth::maximum_type{bandwidth_factor * num_uls});
}

TEST_CASE("Specifying the tag bandwidth overrides the PTW's bandwidth factor")
{
#if (USER_CODES == ENABLE)
  // This fork's VirtualMemory takes the physical memory size directly rather than a
  // MEMORY_CONTROLLER reference, so the controller upstream constructs solely to supply
  // dram.size() is unnecessary here. 1 << 29 bytes is the size that controller slices to.
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, champsim::data::bytes{1ull << 29}};
#else
  MEMORY_CONTROLLER dram{champsim::chrono::picoseconds{3200},
                         champsim::chrono::picoseconds{6400},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{18},
                         std::size_t{38},
                         champsim::chrono::microseconds{64000},
                         {},
                         64,
                         64,
                         1,
                         champsim::data::bytes{8},
                         1024,
                         1024,
                         4,
                         4,
                         4,
                         8192};
  VirtualMemory vmem{champsim::data::bytes{1 << 12}, 4, std::chrono::nanoseconds{6400}, dram};
#endif /* USER_CODES */

  auto num_uls = 2u;
  auto bandwidth_factor = 2u;
  champsim::ptw_builder buildA{};
  std::vector<champsim::channel> channels{num_uls};
  std::vector<champsim::channel*> channel_pointers{};
  for (auto& elem : channels)
    channel_pointers.push_back(&elem);
  buildA.upper_levels(std::move(channel_pointers));
  buildA.bandwidth_factor((double)bandwidth_factor);

  buildA.tag_bandwidth(champsim::bandwidth::maximum_type{6});
  buildA.fill_bandwidth(champsim::bandwidth::maximum_type{7});

  PageTableWalker uut{buildA.virtual_memory(&vmem)};

  CHECK(uut.MAX_READ == champsim::bandwidth::maximum_type{6});
  CHECK(uut.MAX_FILL == champsim::bandwidth::maximum_type{7});
}

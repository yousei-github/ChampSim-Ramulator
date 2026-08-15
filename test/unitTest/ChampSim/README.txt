This directory includes the testing infrastructure for ChampSim. The tests rely on CATCH2, a publicly available framework.

The files are organized as follows: "xxx-descriptive-name.cc", where "xxx" is a three-digit number whose value indicates the portion of the simulator being tested.

000 - Reserved: main function
001-099 - Top-level and utility functionality
100-199 - Core front-end
200-299 - Execution core (out-of-order)
300-399 - Core retirement
400-499 - Caches
600-699 - Page table walkers
700-799 - DRAM
800-899 - Virtual Memory
900-999 - Peculiar and assorted bugs


NOTES FOR THIS FORK
===================

These files were ported from upstream ChampSim (test/cpp/src, commit 51588e1d). Upstream builds them
with its own Makefile ('make test'); here they are a CMake target. Build and run them with:

    cmake --preset tests
    cmake --build --preset tests
    ctest --preset tests                 # every TEST_CASE / SCENARIO as its own ctest entry

Or run the binary directly, which is how upstream's CI invokes it:

    ./test/bin/unit_tests --order rand --warn NoAssertions --invisibles

The three-digit prefix doubles as a Catch2 tag when filename tags are enabled, so a single file can
be selected with:

    ./test/bin/unit_tests -# "[#401-hit-latency]"

Conventions used when porting, worth preserving if you sync with upstream again:

- Divergences from the upstream source sit inside the project's guard convention,
      #if (USER_CODES == ENABLE) ... fork version ... #else ... upstream original ... #endif
  so the upstream text stays readable and re-syncs stay diffable. In practice the divergence is
  almost always just the include prefix ("cache.h" becomes "ChampSim/cache.h").
- The files are otherwise kept in upstream's formatting. Do not run clang-format over them; a
  wholesale reformat would bury the real differences from upstream.
- 000-test-main.cc does not define NUM_CPUS / BLOCK_SIZE / PAGE_SIZE the way upstream's does. This
  fork defines them in ChampSim/champsim_constants.h, so the file only static_asserts the values the
  tests assume: one core, 64-byte lines, 4 KiB pages.
- The PTW and virtual memory tests construct VirtualMemory with a size rather than a MEMORY_CONTROLLER,
  matching this fork's constructor. 1 << 29 bytes is the size upstream's test controller works out to.

Tests 700, 701, 702, 750 and 751 exercise ChampSim's own DRAM model, which RAMULATOR and RAMULATOR2
replace with the Ramulator bridge. They are guarded and compile to nothing unless both toggles are
DISABLE in include/ProjectConfiguration.h, so a default (Ramulator 2.0) build runs 607 test cases and
a ChampSim-internal-memory build runs 624. Both configurations pass; changing the toggle forces a full
rebuild, which ccache makes cheap on the way back.

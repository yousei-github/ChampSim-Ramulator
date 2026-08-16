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

These files were ported from upstream ChampSim (test/cpp/src, commit [51588e1d](https://github.com/ChampSim/ChampSim/commit/51588e1d6f97875fe8de1a3621d28668bff83fcf)). Upstream builds them
with its own Makefile ('make test'); here they are a CMake target. See ../README.md for how to build
and run them, and for the effect of the Ramulator toggles on the DRAM tests (700-751).

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
- Tests 700, 701, 702, 750 and 751 exercise ChampSim's own DRAM model, so they are wrapped in
      #if (RAMULATOR != ENABLE) && (RAMULATOR2 != ENABLE)
  and compile to nothing when either Ramulator backend is in use. 798 needs only dram_stats and the
  printer, which exist in every mode, so it is left unguarded.

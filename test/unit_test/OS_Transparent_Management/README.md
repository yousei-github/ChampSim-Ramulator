# OS-transparent management unit tests

Tests for the research proposals in `source/OS_Transparent_Management/`, which decide where data
lives in a hybrid memory system and when it moves.

## Running them

The suite is part of the `champsim_unit_tests` executable, so it is built and run the same way as
the ChampSim suite next to it (see `test/unit_test/README.md`):

```sh
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
./test/bin/champsim_unit_tests -c "[the scenario name]"   # or run the binary directly
```

## Which tests compile

A research proposal only exists when it is selected in `include/ProjectConfiguration.h`: the
proposal macros are mutually exclusive, and the whole subsystem needs `MEMORY_USE_HYBRID`. Each
test file therefore carries the same guards as the code it exercises and compiles to nothing when
that proposal is not the one selected — the convention DRAM tests 700-751 already use for
ChampSim's own memory model.

The checked-in configuration has `MEMORY_USE_HYBRID` disabled, so **none of these tests are built
by default**. To run them, enable hybrid memory and pick a proposal, then rebuild:

| Configuration in `ProjectConfiguration.h` | Tests that run | Total cases in the binary |
|---|---|---|
| `MEMORY_USE_HYBRID` + `IDEAL_LINE_LOCATION_TABLE` | 100, 200, 201, 202 | 614 |
| `MEMORY_USE_HYBRID` + `COLOCATED_LINE_LOCATION_TABLE` | 100, 200, 201, 202, 210 | 616 |
| `MEMORY_USE_HYBRID` + `IDEAL_VARIABLE_GRANULARITY` | 100, 300, 301 | 613 |
| `MEMORY_USE_HYBRID` + `IDEAL_SINGLE_MEMPOD` | 100, 400, 401 | 614 |
| `MEMORY_USE_HYBRID`, no proposal enabled | 100, 500 | 611 |

The totals include the 607 ChampSim cases the suite next door contributes.

## Numbering

| Range | Area |
|---|---|
| 100-199 | `OS_TRANSPARENT_MANAGEMENT_BASE`, shared by every proposal |
| 200-299 | CAMEO (`IDEAL_LINE_LOCATION_TABLE` / `COLOCATED_LINE_LOCATION_TABLE`); 210+ is co-located only |
| 300-399 | Variable granularity (`IDEAL_VARIABLE_GRANULARITY`) |
| 400-499 | MemPod (`IDEAL_SINGLE_MEMPOD`) |
| 500-599 | The static placement baseline (no proposal enabled) |

## Conventions

Unlike `test/unit_test/ChampSim/`, these files are not ported from upstream, so **do** format them
with clang-format.

`test_capacities.hpp` holds the hybrid memory the tests construct their design under test with
(1 MiB fast, 4 MiB total) and the helpers that turn a congruence group and a memory tier into a
physical address. Both capacities must stay powers of two, since a design derives
`fast_memory_offset_bit` from `champsim::lg2(fast_memory_max_address)`.

Drive a design through its public interface rather than reaching into its tables where you can:
the tables are public, but a test written against `physical_to_hardware_address()` keeps working
when a proposal changes how it stores its mapping. Avoid inputs that make a design call
`std::abort()` (`finish_remapping_request()` on an empty queue, a request whose two addresses are
equal): those paths are sanity checks against a broken memory controller, not behaviour to pin down.

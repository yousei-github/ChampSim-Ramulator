# Unit tests

Catch2 test suites covering the simulator's own code, from address arithmetic and
cache behaviour to the page table walker. They link the very object files the
simulator is built from (the `champsim_core` library), so they exercise the real
classes rather than a copy.

Everything builds into a single `unit_tests` executable. The suites under
[ChampSim/](ChampSim/) are ported from upstream ChampSim; see
[ChampSim/README.txt](ChampSim/README.txt) for their numbering scheme and the
conventions the port follows.

## Quick start

The tests are excluded from the default build. The `tests` preset turns them on and
shares its build directory with the `default` preset, so the simulator objects are
reused instead of recompiled:

```sh
cd ChampSim-Ramulator
cmake --preset tests
cmake --build --preset tests        # -> test/bin/unit_tests
ctest --preset tests                # run every test case
```

`ctest` registers each `TEST_CASE` / `SCENARIO` individually, so a failure names the
one case that broke and `--output-on-failure` (already set by the preset) prints only
its output.

In VS Code, the **CTest: Run ChampSim unit test** task runs the same command, and
**CMake: Configure (preset)** / **CMake: Build (preset)** accept `tests` from their
preset picker.

## Running the binary directly

`ctest` never builds, so rebuild first if you have changed anything — otherwise you
are testing the previous binary.

```sh
./test/bin/unit_tests                                    # everything
./test/bin/unit_tests --order rand --warn NoAssertions --invisibles   # as upstream CI runs it
```

`--order rand` checks that the suite does not depend on execution order, and
`--warn NoAssertions` flags any test case that asserts nothing.

Each file is tagged with its own name, so a single one can be selected with `-#`:

```sh
./test/bin/unit_tests -# "[#401-hit-latency]"
```

Catch2's usual filters work too — by name, `./test/bin/unit_tests "An address is constructible*"`,
and `--list-tests` to see what is registered.

## Simulator configuration affects the suite

The tests compile against the toggles in
[include/ProjectConfiguration.h](../../include/ProjectConfiguration.h), so the build
mode decides which of them exist.

Tests 700, 701, 702, 750 and 751 cover ChampSim's built-in DRAM model, which the
Ramulator bridge replaces. They are guarded and compile to nothing unless both
`RAMULATOR` and `RAMULATOR2` are `DISABLE`:

| Configuration | Test cases |
| --- | --- |
| Ramulator 2.0 (default) | 607 |
| ChampSim internal memory (`RAMULATOR` and `RAMULATOR2` both `DISABLE`) | 624 |

Both configurations pass. Changing a toggle forces a wide recompile, which ccache
makes cheap on the way back.

## Adding another suite

`CMakeLists.txt` here owns the `unit_tests` executable and the Catch2 wiring; each
subdirectory attaches its own sources to it. A new suite — for Ramulator, say — is a
sibling of `ChampSim/` containing a `CMakeLists.txt` of the form

```cmake
target_sources(unit_tests
    PRIVATE
    <files>)
```

added to this directory's `CMakeLists.txt` with `add_subdirectory(<name>)`. Only one
translation unit may define the Catch2 constants that `ChampSim/000-test-main.cc`
already provides, so a new suite contributes test files alone.

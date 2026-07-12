# End-to-end tests

Cross-platform (Linux/macOS/Windows) pytest suite that runs the
`champsim_plus_ramulator` binary on a real trace with tiny instruction counts and
checks the run succeeded and emitted sane statistics.

The suite does **not** build anything. You build the simulator (VS Code or CMake),
then pytest locates the executable and runs it, auto-detecting the compiled mode
from `include/ProjectConfiguration.h` so the matching config-file arguments are
passed automatically.

## Requirements

```sh
cd ChampSim-Ramulator
python -m venv .virtualPythonEnvironment          # create a virtual environment (.virtualPythonEnvironment)
source .virtualPythonEnvironment/bin/activate     # activate created virtual environment
pip install -r test/end_to_end/requirements.txt   # just pytest
deactivate                                        # deactivate the virtual environment
```

You also need a built binary and at least one trace (see below).

## Quick start

```sh
cd ChampSim-Ramulator
cmake --preset default && cmake --build --preset default   # -> bin/champsim_plus_ramulator
source .virtualPythonEnvironment/bin/activate              # activate created virtual environment
python -m pytest -vv test/end_to_end
deactivate                                                 # deactivate the virtual environment
```

Or build with the VS Code **CMake: Build (preset)** task (or **Build by specified
optimization**), then run the `pytest` command above.

It only exercises the mode the binary was actually built with. To cover another
mode, change the toggles in `include/ProjectConfiguration.h`, rebuild, and run
pytest again.

All output files (`.statistics`, `.trace`) are written to pytest's temp dir, so
nothing lands in the repository.

## Choosing the binary

By default the suite runs `bin/champsim_plus_ramulator`. `bin/` may hold several
binaries; set `CHAMPSIM_BINARY` to a full path to test a specific one:

```sh
CHAMPSIM_BINARY=$PWD/bin/champsim_plus_ramulator2 python -m pytest -vv test/end_to_end
```

If `ProjectConfiguration.h` is newer than the binary, the suite warns that the
binary may be stale — rebuild so it matches the auto-detected mode.

## Traces

Traces are large and not committed. Resolution order:

1. `CHAMPSIM_TRACE` — full path to a specific `.champsimtrace.xz`.
2. `CHAMPSIM_TRACE_DIR` — directory to search (defaults to the conventional
   `dpc3_traces` locations). Uses `603.bwaves_s-3699B.champsimtrace.xz` if
   present, else the first trace found.

If no trace is found, the tests **skip** (they do not fail).

```sh
CHAMPSIM_TRACE=/path/to/some.champsimtrace.xz python -m pytest -vv test/end_to_end
```

## Instruction counts

Defaults are tiny (50k warmup / 50k simulation) for speed. Override with:

```sh
python -m pytest test/end_to_end --warmup 100000 --sim 100000
```

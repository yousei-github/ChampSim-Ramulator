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
```

You also need a built binary and at least one trace (see below).

## Quick start

```sh
cmake --preset default && cmake --build --preset default   # -> bin/champsim_plus_ramulator
python -m pytest -vv test/end_to_end                       # Execute end-to-end tests
deactivate                                                 # deactivate the virtual environment if need
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
binaries; pass `--binary` to test a specific one. A relative path is resolved
against the `ChampSim-Ramulator/` directory (an absolute path is used as-is):

```sh
python -m pytest -vv test/end_to_end --binary bin/champsim_plus_ramulator
```

If `ProjectConfiguration.h` is newer than the binary, the suite warns that the
binary may be stale — rebuild so it matches the auto-detected mode.

## Traces

Traces are large and not committed. Two options select the trace:

1. `--trace-file` — the trace to run (default `603.bwaves_s-3699B.champsimtrace.xz`).
   A relative path is resolved against the trace directory; an absolute path is used as-is.
2. `--trace-directory` — where relative trace files live (default `dpc3_traces`,
   resolved against `ChampSim-Ramulator/`).

If the resolved trace is missing, the tests **skip** (they do not fail).

```sh
python -m pytest -vv test/end_to_end --trace-file path/to/some.champsimtrace.xz
```

## Instruction counts

Defaults are tiny (50k warmup / 50k simulation) for speed. Override with:

```sh
python -m pytest test/end_to_end --warmup 100000 --simulation 100000
```

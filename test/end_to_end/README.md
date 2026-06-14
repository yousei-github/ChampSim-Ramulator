# End-to-end tests

Cross-platform (Linux/macOS/Windows) pytest suite that runs the
`champsim_plus_ramulator` binary on a real trace with tiny instruction counts and
checks the run succeeded and emitted sane statistics.

## Requirements

```sh
pip install -r test/end_to_end/requirements.txt   # just pytest
```

You also need a built binary and at least one trace (see below).

## Quick start

```sh
cd ChampSim-Ramulator
cmake --build build -j            # ensure bin/champsim_plus_ramulator exists
python -m pytest test/end_to_end -v
```

The suite auto-detects the **compiled mode** from `include/ProjectConfiguration.h`
(ChampSim-only / Ramulator 1.0 / Ramulator 2.0, single vs hybrid) and passes the
matching config-file arguments automatically. It only exercises the mode the
binary was actually built with — see `--matrix` below to cover all of them.

All output files (`.statistics`, `.trace`) are written to pytest's temp dir, so
nothing lands in the repository.

## Traces

Traces are large and not committed. Resolution order:

1. `CHAMPSIM_TRACE` — full path to a specific `.champsimtrace.xz`.
2. `CHAMPSIM_TRACE_DIR` — directory to search (default: `../dpc3_traces`). Uses
   `603.bwaves_s-3699B.champsimtrace.xz` if present, else the first trace found.

If no trace is found, the tests **skip** (they do not fail).

```sh
CHAMPSIM_TRACE=/path/to/some.champsimtrace.xz python -m pytest test/end_to_end -v
```

## Instruction counts

Defaults are tiny (50k warmup / 50k simulation) for speed. Override with:

```sh
python -m pytest test/end_to_end --warmup 100000 --sim 100000
```

## Full mode matrix (opt-in, slow)

`--matrix` rewrites the three mode toggles in `ProjectConfiguration.h`, rebuilds
the binary for each mode into a separate `build_e2e/` dir under the name
`champsim_e2e`, and runs each. The original `ProjectConfiguration.h` is restored
afterward (even on failure). Each mode switch is a wide recompile, so this takes
several minutes.

```sh
python -m pytest test/end_to_end --matrix -v
```

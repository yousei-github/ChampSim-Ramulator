"""End-to-end tests: run the currently-built binary on a real trace and check it.

Build the binary first (the VS Code build task, or `cmake --build --preset default`),
then run `python -m pytest test/end_to_end`. The mode is auto-detected from
ProjectConfiguration.h, so the correct config-file arguments are supplied automatically.

All runs use a tmp_path working directory, so no .statistics/.trace files are left in the repository.
"""

from __future__ import annotations

import warnings
from pathlib import Path

import pytest

from simulator_runner import (
    Mode,
    Execution,
    config_args_for_mode,
    run_simulation,
    parse_statistics,
)


@pytest.fixture(scope="session")
def get_binary_path(find_binary_path: Path, get_repository_root: Path) -> Path:

    # We test whatever was already built; warn (don't fail) if the header changed
    # since the auto-detected mode would then not match the binary.
    HEADER = get_repository_root / "include" / "ProjectConfiguration.h"

    if HEADER.is_file() and HEADER.stat().st_mtime > find_binary_path.stat().st_mtime:
        warnings.warn(
            f"{find_binary_path.name} is older than {HEADER.name}; it may be stale — "
            "rebuild so the tested binary matches the detected mode.",
            stacklevel=2,
        )

    return find_binary_path


@pytest.fixture(scope="session")
def run_result(
    get_binary_path: Path,
    get_current_mode: Mode,
    get_repository_root: Path,
    find_trace: Path,
    get_warmup: int,
    get_simulation: int,
    tmp_path_factory: pytest.TempPathFactory,
) -> Execution:
    """Run the simulator once for the whole module and share the result."""

    CONFIG_FILES = config_args_for_mode(get_current_mode, get_repository_root)
    # Multi-core builds need one trace per core; single-core needs just one.
    TRACES = [find_trace] * (2 if get_current_mode.multicore else 1)
    WORKING_DIRECTORY = tmp_path_factory.mktemp("e2e_run")

    print(f"\nRun simulation in {WORKING_DIRECTORY}")

    RESULT = run_simulation(
        binary=get_binary_path,
        config_files=CONFIG_FILES,
        traces=TRACES,
        warmup=get_warmup,
        simulation=get_simulation,
        workdir=WORKING_DIRECTORY,
    )

    print(f"\nFinish simulation")

    return RESULT

# Test case: mode check


def test_mode_detected(get_current_mode: Mode) -> None:

    # Surfaces the detected mode in -v output for quick diagnosis.
    print(f"\nDetected build mode: {get_current_mode.name}")

    # ProjectConfiguration.h forbids enabling both DRAM backends at once.
    assert not (get_current_mode.ramulator1 and get_current_mode.ramulator2), (
        "RAMULATOR and RAMULATOR2 cannot both be enabled"
    )

# Test case: simulation execution result check


def test_runs_successfully(run_result: Execution) -> None:
    assert (run_result.returncode == 0), (
        f"Non-zero exit ({run_result.returncode}).\n"
        f"argv: {run_result.argv}\n"
        f"stderr tail:\n{run_result.stderr[-2000:]}"
    )
    assert run_result.completed, (
        "Did not find completion marker in stdout.\n"
        f"stdout tail:\n{run_result.stdout[-2000:]}"
    )


def test_statistics_file_written(run_result: Execution) -> None:
    assert run_result.statistics_written, (
        f"Expected non-empty statistics file at {run_result.statistics_path}"
    )


def test_metrics_sane(run_result: Execution) -> None:
    text = run_result.statistics_path.read_text(encoding="utf-8", errors="replace")
    stats = parse_statistics(text)

    assert stats.completed, "Statistics file missing the completion marker."
    assert stats.last_cumulative_ipc is not None, "No cumulative IPC found in statistics."
    assert stats.last_cumulative_ipc > 0.0, (
        f"Cumulative IPC should be positive, got {stats.last_cumulative_ipc}"
    )
    assert stats.l1d_total_access is not None and stats.l1d_total_access > 0, (
        "Expected a positive cpu0 L1D total access count."
    )

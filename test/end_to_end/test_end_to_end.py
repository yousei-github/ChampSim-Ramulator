"""End-to-end tests: run the currently-built binary on a real trace and check it.

Build the binary first (the VS Code build task, or
`cmake --preset default && cmake --build --preset default`), then run
`python -m pytest test/end_to_end`. The mode is auto-detected from
ProjectConfiguration.h, so the correct config-file arguments are supplied
automatically.

All runs use a tmp_path working directory, so no .statistics/.trace files are left
in the repository.
"""

from __future__ import annotations

import warnings
from pathlib import Path

import pytest

from champsim_runner import (
    Mode,
    RunResult,
    config_args_for_mode,
    parse_statistics,
    run_simulation,
)


@pytest.fixture(scope="session")
def binary(find_binary_path: Path, get_repository_root: Path) -> Path:
    if not find_binary_path.is_file():
        pytest.skip(
            f"Binary not built: {find_binary_path}. Build it first — e.g.\n"
            "  cmake --preset default && cmake --build --preset default\n"
            "or the VS Code 'CMake: Build (preset)' task — or set CHAMPSIM_BINARY "
            "to an existing binary."
        )

    # We test whatever was already built; warn (don't fail) if the header changed
    # since the auto-detected mode would then not match the binary.
    header = get_repository_root / "include" / "ProjectConfiguration.h"

    if header.is_file() and header.stat().st_mtime > find_binary_path.stat().st_mtime:
        warnings.warn(
            f"{find_binary_path.name} is older than {header.name}; it may be stale — "
            "rebuild so the tested binary matches the detected mode.",
            stacklevel=2,
        )
    return find_binary_path


@pytest.fixture(scope="session")
def run_result(
    binary: Path,
    current_mode: Mode,
    get_repository_root: Path,
    trace: Path,
    get_warmup: int,
    get_simulation: int,
    tmp_path_factory: pytest.TempPathFactory,
) -> RunResult:
    """Run the simulator once for the whole module and share the result."""
    config_files = config_args_for_mode(current_mode, get_repository_root)
    # Multi-core builds need one trace per core; single-core needs just one.
    traces = [trace] * (2 if current_mode.multicore else 1)
    workdir = tmp_path_factory.mktemp("e2e_run")
    result = run_simulation(
        binary=binary,
        config_files=config_files,
        traces=traces,
        warmup=get_warmup,
        simulation=get_simulation,
        workdir=workdir,
    )
    return result


def test_mode_detected(current_mode: Mode) -> None:
    # Surfaces the detected mode in -v output for quick diagnosis.
    print(f"\nDetected build mode: {current_mode.name}")
    # ProjectConfiguration.h forbids enabling both DRAM backends at once.
    assert not (current_mode.ramulator1 and current_mode.ramulator2), (
        "RAMULATOR and RAMULATOR2 cannot both be enabled"
    )


def test_runs_successfully(run_result: RunResult) -> None:
    assert run_result.returncode == 0, (
        f"Non-zero exit ({run_result.returncode}).\n"
        f"argv: {run_result.argv}\n"
        f"stderr tail:\n{run_result.stderr[-2000:]}"
    )
    assert run_result.completed, (
        "Did not find completion marker in stdout.\n"
        f"stdout tail:\n{run_result.stdout[-2000:]}"
    )


def test_statistics_file_written(run_result: RunResult) -> None:
    assert run_result.statistics_written, (
        f"Expected non-empty statistics file at {run_result.statistics_path}"
    )


def test_metrics_sane(run_result: RunResult) -> None:
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

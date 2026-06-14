"""Smoke + sanity end-to-end tests against the currently-built binary.

These replace the manual `execution*.sh` workflow: build the binary, then run
`python -m pytest test/end_to_end`. The mode is auto-detected from
ProjectConfiguration.h, so the correct config arguments are supplied automatically.

All runs use a tmp_path working directory, so no .statistics/.trace files are left
in the repository.
"""

from __future__ import annotations

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
def binary(binary_path: Path) -> Path:
    if not binary_path.is_file():
        pytest.skip(
            f"Binary not built: {binary_path}. Build first, e.g.\n"
            "  cmake --build build -j\n"
            "or use the VS Code 'CMake: Build (preset)' task."
        )
    return binary_path


@pytest.fixture(scope="session")
def run_result(
    binary: Path,
    current_mode: Mode,
    repo_root: Path,
    trace: Path,
    warmup: int,
    simulation: int,
    tmp_path_factory: pytest.TempPathFactory,
) -> RunResult:
    """Run the simulator once for the whole module and share the result."""
    config_files = config_args_for_mode(current_mode, repo_root)
    traces = [trace] * (2 if current_mode.multicore else 1)
    workdir = tmp_path_factory.mktemp("e2e_run")
    result = run_simulation(
        binary=binary,
        config_files=config_files,
        traces=traces,
        warmup=warmup,
        simulation=simulation,
        workdir=workdir,
    )
    return result


def test_mode_detected(current_mode: Mode) -> None:
    # Surfaces the detected mode in -v output for quick diagnosis.
    print(f"\nDetected build mode: {current_mode.name}")
    assert current_mode.ramulator1 + current_mode.ramulator2 <= 1, (
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


def test_metrics_sane(run_result: RunResult, simulation: int) -> None:
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

"""Shared pytest fixtures and options for the ChampSim-Ramulator end-to-end suite.

Everything here is cross-platform (pathlib + subprocess, no shell). Tests run
against whatever binary is currently built; the simulation mode is auto-detected
from ``include/ProjectConfiguration.h`` so the correct config arguments are passed.
"""

from __future__ import annotations

import os
from pathlib import Path

import pytest

from champsim_runner import Mode, detect_mode

# Default trace + instruction counts. Counts are deliberately tiny: a .champsimtrace.xz
# is streamed, so even a multi-hundred-MB trace finishes quickly with small counts.
DEFAULT_TRACE_NAME = "603.bwaves_s-3699B.champsimtrace.xz"
DEFAULT_WARMUP = 50_000
DEFAULT_SIMULATION = 50_000


def pytest_addoption(parser: pytest.Parser) -> None:
    parser.addoption(
        "--matrix",
        action="store_true",
        default=False,
        help="Run the opt-in mode matrix: rewrite ProjectConfiguration.h, rebuild "
        "the binary for each mode, and run it (slow; many full recompiles).",
    )
    parser.addoption(
        "--warmup",
        type=int,
        default=DEFAULT_WARMUP,
        help=f"Warmup instructions per run (default: {DEFAULT_WARMUP}).",
    )
    parser.addoption(
        "--sim",
        type=int,
        default=DEFAULT_SIMULATION,
        help=f"Simulation instructions per run (default: {DEFAULT_SIMULATION}).",
    )


def pytest_configure(config: pytest.Config) -> None:
    config.addinivalue_line(
        "markers", "matrix: opt-in test that rebuilds the binary for each mode (slow)."
    )


@pytest.fixture(scope="session")
def repo_root() -> Path:
    """The ChampSim-Ramulator/ directory (two levels up from this file)."""
    return Path(__file__).resolve().parents[2]


@pytest.fixture(scope="session")
def binary_path(repo_root: Path) -> Path:
    name = "champsim_plus_ramulator" + (".exe" if os.name == "nt" else "")
    return repo_root / "bin" / name


@pytest.fixture(scope="session")
def warmup(request: pytest.FixtureRequest) -> int:
    return int(request.config.getoption("--warmup"))


@pytest.fixture(scope="session")
def simulation(request: pytest.FixtureRequest) -> int:
    return int(request.config.getoption("--sim"))


@pytest.fixture(scope="session")
def trace_dir(repo_root: Path) -> Path:
    """Directory holding .champsimtrace.xz files. Override with CHAMPSIM_TRACE_DIR.

    By default, search the conventional `dpc3_traces` locations relative to the
    repo (a sibling of ChampSim-Ramulator/, or one level further up). Returns the
    first that exists; falls back to the sibling location for the skip message.
    """
    env = os.environ.get("CHAMPSIM_TRACE_DIR")
    if env:
        return Path(env)
    candidates = [
        repo_root.parent / "dpc3_traces",        # CLAUDE.md's documented ../dpc3_traces
        repo_root.parent.parent / "dpc3_traces",  # actual location in this checkout
    ]
    for candidate in candidates:
        if candidate.is_dir():
            return candidate.resolve()
    return candidates[0].resolve()


@pytest.fixture(scope="session")
def trace(trace_dir: Path) -> Path:
    """Resolve a trace to run against, skipping the suite if none is available.

    Resolution order:
      1. CHAMPSIM_TRACE (explicit full path to a trace),
      2. <trace_dir>/<DEFAULT_TRACE_NAME>,
      3. the first *.champsimtrace.xz found in <trace_dir>.
    """
    explicit = os.environ.get("CHAMPSIM_TRACE")
    if explicit:
        path = Path(explicit)
        if not path.is_file():
            pytest.skip(f"CHAMPSIM_TRACE points at a missing file: {path}")
        return path

    if not trace_dir.is_dir():
        pytest.skip(
            f"Trace directory not found: {trace_dir}. Set CHAMPSIM_TRACE_DIR or "
            "CHAMPSIM_TRACE to point at a .champsimtrace.xz file."
        )

    default = trace_dir / DEFAULT_TRACE_NAME
    if default.is_file():
        return default

    candidates = sorted(trace_dir.glob("*.champsimtrace.xz"))
    if not candidates:
        pytest.skip(f"No *.champsimtrace.xz files found in {trace_dir}.")
    return candidates[0]


@pytest.fixture(scope="session")
def current_mode(repo_root: Path) -> Mode:
    return detect_mode(repo_root)

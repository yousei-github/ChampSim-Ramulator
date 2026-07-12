# (1) The conftest.py file serves as a means of providing fixtures for an entire directory.
# Fixtures defined in a conftest.py can be used by any test in that package without needing to import them (pytest will automatically discover them).
# (2) You can also use the conftest.py file to implement local per-directory plugins.
# Local conftest.py plugins contain directory-specific hook implementations.
# Hook Session and test running activities will invoke all hooks defined in conftest.py files closer to the root of the filesystem.

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

# Options
WARMUP_OPTIONS = "--warmup"
SIMULATION_OPTIONS = "--sim"
DEFAULT_WARMUP_VALUE = 50_000
DEFAULT_SIMULATION_VALUE = 50_000


# Hook: add options


def pytest_addoption(parser: pytest.Parser) -> None:
    """
    Register argparse-style options and config-style config values, called once at the beginning of a test run.

    Parameters:
    To add command line options, call parser.addoption(...).
    To add config-file values call parser.addini(...).

    Options can later be accessed through the config object, respectively:
    - config.getoption(name) to retrieve the value of a command line option.
    - config.getini(name) to retrieve a value read from a configuration file.
    """

    parser.addoption(
        WARMUP_OPTIONS,
        type=int,
        default=DEFAULT_WARMUP_VALUE,
        help=f"Warmup instructions per run (default: {DEFAULT_WARMUP_VALUE}).",
    )
    parser.addoption(
        SIMULATION_OPTIONS,
        type=int,
        default=DEFAULT_SIMULATION_VALUE,
        help=f"Simulation instructions per run (default: {DEFAULT_SIMULATION_VALUE}).",
    )

# Fixtures


@pytest.fixture(scope="session")
def get_warmup(request: pytest.FixtureRequest) -> int:
    """
    Return command line option WARMUP_OPTIONS's value

    Fixture functions can accept the request object to introspect the "requesting" test function, class or module context.
    The request fixture is a special fixture providing information of the requesting test function.
    """
    return int(request.config.getoption(WARMUP_OPTIONS))


@pytest.fixture(scope="session")
def get_simulation(request: pytest.FixtureRequest) -> int:
    """
    Return command line option SIMULATION_OPTIONS's value

    Fixture functions can accept the request object to introspect the "requesting" test function, class or module context.
    The request fixture is a special fixture providing information of the requesting test function.
    """
    return int(request.config.getoption(SIMULATION_OPTIONS))


@pytest.fixture(scope="session")
def get_repository_root() -> Path:
    """Get the ChampSim-Ramulator/ directory (two levels up from this file)."""
    return Path(__file__).resolve().parents[2]


@pytest.fixture(scope="session")
def find_binary_path(get_repository_root: Path) -> Path:
    """The simulator executable to test.

    Set CHAMPSIM_BINARY to a full path to test a specific binary (bin/ can hold
    several, built with different modes); otherwise default to the conventional
    bin/champsim_plus_ramulator produced by the VS Code / CMake build.
    """
    binary_path = os.environ.get("CHAMPSIM_BINARY")
    if binary_path:
        # binary path is overrode by user
        # Todo: Pass different values to a test function, depending on command line options
        binary_path = Path(binary_path)
    else:
        name = "champsim_plus_ramulator" + (".exe" if os.name == "nt" else "")
        binary_path = get_repository_root / "bin" / name

    print(f"\nUse binary: {binary_path}")
    return binary_path


@pytest.fixture(scope="session")
def find_trace_directory(get_repository_root: Path) -> Path:
    """Directory holding .champsimtrace.xz files. Override with CHAMPSIM_TRACE_DIR.

    By default, search the conventional `dpc3_traces` locations relative to the
    repository (a sibling of ChampSim-Ramulator/, or one level further up). Returns the
    first that exists; falls back to the sibling location for the skip message.
    """
    env = os.environ.get("CHAMPSIM_TRACE_DIR")
    if env:
        return Path(env)

    candidates = [
        get_repository_root.parent / "dpc3_traces",        # CLAUDE.md's documented ../dpc3_traces
        get_repository_root.parent.parent / "dpc3_traces",  # actual location in this checkout
    ]
    for candidate in candidates:
        if candidate.is_dir():
            return candidate.resolve()

    return candidates[0].resolve()


@pytest.fixture(scope="session")
def trace(find_trace_directory: Path) -> Path:
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

    if not find_trace_directory.is_dir():
        pytest.skip(
            f"Trace directory not found: {find_trace_directory}. Set CHAMPSIM_TRACE_DIR or "
            "CHAMPSIM_TRACE to point at a .champsimtrace.xz file."
        )

    default = find_trace_directory / DEFAULT_TRACE_NAME
    if default.is_file():
        return default

    candidates = sorted(find_trace_directory.glob("*.champsimtrace.xz"))
    if not candidates:
        pytest.skip(f"No *.champsimtrace.xz files found in {find_trace_directory}.")
    return candidates[0]


@pytest.fixture(scope="session")
def current_mode(get_repository_root: Path) -> Mode:
    """The compile-time mode the binary was built with, read from ProjectConfiguration.h.

    The suite never modifies the header, so reading it live is safe and tells us
    which config-file arguments the binary expects.
    """
    return detect_mode(get_repository_root)

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

from simulator_runner import Mode, detect_mode

# Options
BINARY_OPTIONS = "--binary"
WARMUP_OPTIONS = "--warmup"
SIMULATION_OPTIONS = "--simulation"
TRACE_DIRECTORY_OPTIONS = "--trace-directory"
TRACE_FILE_OPTIONS = "--trace-file"

# Option default values
DEFAULT_BINARY_PATH = "bin/champsim_plus_ramulator" + (".exe" if os.name == "nt" else "")  # Binary path option default
DEFAULT_WARMUP_VALUE = 50_000  # Default warmup instruction counts
DEFAULT_SIMULATION_VALUE = 50_000  # Default simulation instruction counts
DEFAULT_TRACE_DIRECTORY = "../../dpc3_traces"  # Trace directory path option default
DEFAULT_TRACE_NAME = "603.bwaves_s-3699B.champsimtrace.xz"  # Trace file path option default


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
    parser.addoption(
        BINARY_OPTIONS,
        default=DEFAULT_BINARY_PATH,
        help=f"Simulator binary to test (default: {DEFAULT_BINARY_PATH}).",
    )
    parser.addoption(
        TRACE_FILE_OPTIONS,
        default=DEFAULT_TRACE_NAME,
        help=f"Trace to run; relative to the trace directory (default: {DEFAULT_TRACE_NAME}).",
    )
    parser.addoption(
        TRACE_DIRECTORY_OPTIONS,
        default=DEFAULT_TRACE_DIRECTORY,
        help=f"Directory holding .champsimtrace.xz files (default: {DEFAULT_TRACE_DIRECTORY}).",
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
def find_binary_path(request: pytest.FixtureRequest, get_repository_root: Path) -> Path:
    """The simulator executable to test (default: DEFAULT_BINARY_PATH).

    Pass --binary to test a specific binary (bin/ can hold several, built with
    different modes); a relative path is resolved against the ChampSim-Ramulator/ directory.
    """
    binary_path = Path(request.config.getoption(BINARY_OPTIONS))
    if not binary_path.is_absolute():
        binary_path = get_repository_root / binary_path
    if not binary_path.is_file():
        pytest.skip(
            f"Executable not found: {binary_path}. Pass --binary "
            "to point at an executable file."
        )

    print(f"\nUse binary: {binary_path}")
    return binary_path


@pytest.fixture(scope="session")
def find_trace_directory(request: pytest.FixtureRequest, get_repository_root: Path) -> Path:
    """Directory holding .champsimtrace.xz files (default: DEFAULT_TRACE_DIRECTORY).

    Pass --trace-directory to override; a relative path (including the default) is
    resolved against the ChampSim-Ramulator/ directory.
    """
    trace_directory_path = Path(request.config.getoption(TRACE_DIRECTORY_OPTIONS))
    if not trace_directory_path.is_absolute():
        trace_directory_path = get_repository_root / trace_directory_path
    if not trace_directory_path.is_dir():
        pytest.skip(
            f"Trace directory not found: {trace_directory_path}. Pass --trace-directory "
            "to point at a directory."
        )

    print(f"\nUse trace directory: {trace_directory_path}")
    return trace_directory_path.resolve()


@pytest.fixture(scope="session")
def find_trace(request: pytest.FixtureRequest, find_trace_directory: Path) -> Path:
    """The trace to run against, skipping the suite if it is missing.

    Pass --trace-file to override; a relative path (including the default) is
    resolved against the trace directory, an absolute path is used as-is.
    """
    trace_file_path = Path(request.config.getoption(TRACE_FILE_OPTIONS))
    if not trace_file_path.is_absolute():
        trace_file_path = find_trace_directory / trace_file_path
    if not trace_file_path.is_file():
        pytest.skip(
            f"Trace not found: {trace_file_path}. Pass --trace-file / --trace-directory "
            "to point at a .champsimtrace.xz file."
        )

    print(f"\nUse trace: {trace_file_path}")
    return trace_file_path


@pytest.fixture(scope="session")
def get_current_mode(get_repository_root: Path) -> Mode:
    """The compile-time mode the binary was built with, read from ProjectConfiguration.h.

    The suite never modifies the header, so reading it live is safe and tells us
    which config-file arguments the binary expects.
    """
    return detect_mode(get_repository_root)

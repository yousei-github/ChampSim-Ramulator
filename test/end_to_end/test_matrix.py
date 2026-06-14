"""Opt-in full mode-matrix end-to-end tests.

Because the simulation mode is a compile-time decision, covering every mode means
rewriting ``include/ProjectConfiguration.h`` and rebuilding the binary for each
one. That is slow (each mode switch is a wide recompile), so these tests only run
when ``--matrix`` is passed:

    cd ChampSim-Ramulator
    python -m pytest test/end_to_end --matrix -v

The original ProjectConfiguration.h is backed up and restored (even on failure),
and each mode is built into a dedicated ``build_e2e`` directory under a separate
executable name (``champsim_e2e``) so the user's main binary and build are never
disturbed.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest

from champsim_runner import (
    Mode,
    config_args_for_mode,
    parse_statistics,
    project_configuration_path,
    run_simulation,
)

pytestmark = pytest.mark.matrix

# (test id, Mode). multicore stays disabled for the matrix.
MATRIX_MODES = [
    pytest.param(Mode(False, False, False, False), id="champsim_only"),
    pytest.param(Mode(True, False, False, False), id="ramulator1_single"),
    pytest.param(Mode(True, False, True, False), id="ramulator1_hybrid"),
    pytest.param(Mode(False, True, False, False), id="ramulator2_single"),
    pytest.param(Mode(False, True, True, False), id="ramulator2_hybrid"),
]

E2E_BUILD_DIR = "build_e2e"
E2E_EXECUTABLE = "champsim_e2e"


def _bool_macro(value: bool) -> str:
    return "ENABLE" if value else "DISABLE"


def _rewrite_toggles(text: str, mode: Mode) -> str:
    """Set only the three mode toggles; leave everything else untouched.

    The ``\\s+`` after each macro name prevents ``RAMULATOR`` from matching the
    longer ``RAMULATOR2`` token.
    """
    replacements = {
        "RAMULATOR": mode.ramulator1,
        "RAMULATOR2": mode.ramulator2,
        "MEMORY_USE_HYBRID": mode.hybrid,
    }
    for macro, enabled in replacements.items():
        pattern = re.compile(rf"(#define\s+{macro}\s+\()(?:ENABLE|DISABLE)(\))")
        text, n = pattern.subn(rf"\g<1>{_bool_macro(enabled)}\g<2>", text)
        if n != 1:
            raise AssertionError(f"Expected exactly one {macro} toggle, replaced {n}.")
    return text


@pytest.fixture
def _config_backup(request: pytest.FixtureRequest, repo_root: Path, original_config: str):
    """Per-test guard for the matrix tests.

    Function-scoped so ProjectConfiguration.h is restored to the session-start
    snapshot after *each* mode, never left mutated for later tests (notably the
    smoke suite, which auto-detects the mode from that same file). The
    ``original_config`` session fixture provides the snapshot and a final safety-net
    restore at session end.
    """
    if not request.config.getoption("--matrix"):
        pytest.skip("matrix tests require --matrix")
    config = project_configuration_path(repo_root)
    try:
        yield config
    finally:
        config.write_text(original_config, encoding="utf-8")


def _build(repo_root: Path) -> Path:
    """Configure (idempotent) and build the e2e binary; return its path."""
    toolchain = repo_root / "external" / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake"
    subprocess.run(
        [
            "cmake", "-S", os.fspath(repo_root), "-B", os.fspath(repo_root / E2E_BUILD_DIR),
            f"-DCMAKE_TOOLCHAIN_FILE={os.fspath(toolchain)}",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DEXECUTABLE_NAME={E2E_EXECUTABLE}",
        ],
        cwd=os.fspath(repo_root),
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", os.fspath(repo_root / E2E_BUILD_DIR), "-j", str(os.cpu_count() or 1)],
        cwd=os.fspath(repo_root),
        check=True,
    )
    binary = repo_root / "bin" / (E2E_EXECUTABLE + (".exe" if os.name == "nt" else ""))
    assert binary.is_file(), f"Build did not produce {binary}"
    return binary


@pytest.mark.parametrize("mode", MATRIX_MODES)
def test_mode(
    mode: Mode,
    _config_backup: Path,
    repo_root: Path,
    trace: Path,
    warmup: int,
    simulation: int,
    tmp_path: Path,
) -> None:
    if shutil.which("cmake") is None:
        pytest.skip("cmake not found on PATH")

    # Rewrite toggles for this mode, then rebuild.
    config = _config_backup
    config.write_text(_rewrite_toggles(config.read_text(encoding="utf-8"), mode), encoding="utf-8")
    binary = _build(repo_root)

    traces = [trace] * (2 if mode.multicore else 1)
    result = run_simulation(
        binary=binary,
        config_files=config_args_for_mode(mode, repo_root),
        traces=traces,
        warmup=warmup,
        simulation=simulation,
        workdir=tmp_path,
    )

    assert result.returncode == 0, (
        f"[{mode.name}] non-zero exit ({result.returncode}).\n"
        f"stderr tail:\n{result.stderr[-2000:]}"
    )
    assert result.completed, f"[{mode.name}] missing completion marker in stdout."
    assert result.statistics_written, (
        f"[{mode.name}] expected non-empty statistics at {result.statistics_path}"
    )
    stats = parse_statistics(result.statistics_path.read_text(encoding="utf-8", errors="replace"))
    assert stats.last_cumulative_ipc and stats.last_cumulative_ipc > 0.0, (
        f"[{mode.name}] cumulative IPC should be positive, got {stats.last_cumulative_ipc}"
    )

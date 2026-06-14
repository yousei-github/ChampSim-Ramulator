"""Reusable helpers for driving the ChampSim-Ramulator binary from tests.

This module is intentionally free of any pytest dependency so it can be reused
by ad-hoc scripts as well. Everything uses pathlib + subprocess so it works the
same on Linux, macOS, and Windows.

The simulation *mode* (ChampSim-only / Ramulator 1.0 / Ramulator 2.0, single vs
hybrid memory) is a compile-time decision baked into the binary via the macros in
``include/ProjectConfiguration.h``. A given binary therefore only accepts the
config-file arguments that match the mode it was compiled with; ``Mode`` below
describes that, and ``config_args_for_mode`` produces the matching argument list.
"""

from __future__ import annotations

import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

# Marker the binary prints to stdout once every CPU has finished its phases.
COMPLETION_MARKER = "ChampSim completed all CPUs"


@dataclass(frozen=True)
class Mode:
    """Describes the compile-time configuration the binary was built with."""

    ramulator1: bool       # RAMULATOR  == ENABLE
    ramulator2: bool       # RAMULATOR2 == ENABLE
    hybrid: bool           # MEMORY_USE_HYBRID == ENABLE
    multicore: bool        # CPU_USE_MULTIPLE_CORES == ENABLE

    @property
    def name(self) -> str:
        if self.ramulator1:
            backend = "ramulator1"
        elif self.ramulator2:
            backend = "ramulator2"
        else:
            backend = "champsim_only"
        memory = "hybrid" if self.hybrid else "single"
        cores = "_multicore" if self.multicore else ""
        if backend == "champsim_only":
            return f"{backend}{cores}"
        return f"{backend}_{memory}{cores}"


# Toggle macros we care about, matched as e.g. ``#define RAMULATOR2 (ENABLE)``.
_TOGGLE_RE = {
    "ramulator1": re.compile(r"^\s*#define\s+RAMULATOR\s+\((ENABLE|DISABLE)\)", re.M),
    "ramulator2": re.compile(r"^\s*#define\s+RAMULATOR2\s+\((ENABLE|DISABLE)\)", re.M),
    "hybrid": re.compile(r"^\s*#define\s+MEMORY_USE_HYBRID\s+\((ENABLE|DISABLE)\)", re.M),
    "multicore": re.compile(r"^\s*#define\s+CPU_USE_MULTIPLE_CORES\s+\((ENABLE|DISABLE)\)", re.M),
}


def project_configuration_path(repo_root: Path) -> Path:
    return repo_root / "include" / "ProjectConfiguration.h"


def detect_mode_from_text(text: str) -> Mode:
    """Parse the contents of ``ProjectConfiguration.h`` into a ``Mode``."""
    values: dict[str, bool] = {}
    for key, pattern in _TOGGLE_RE.items():
        match = pattern.search(text)
        if match is None:
            raise ValueError(f"Could not find toggle for {key} in ProjectConfiguration.h")
        values[key] = match.group(1) == "ENABLE"
    return Mode(
        ramulator1=values["ramulator1"],
        ramulator2=values["ramulator2"],
        hybrid=values["hybrid"],
        multicore=values["multicore"],
    )


def detect_mode(repo_root: Path) -> Mode:
    """Parse ``ProjectConfiguration.h`` on disk to learn how the binary was compiled."""
    text = project_configuration_path(repo_root).read_text(encoding="utf-8", errors="replace")
    return detect_mode_from_text(text)


def config_args_for_mode(mode: Mode, repo_root: Path) -> list[Path]:
    """Absolute config-file arguments required by ``mode``.

    Mirrors the documented run commands in CLAUDE.md:
      * ChampSim-only      -> no config file
      * Ramulator 1.0      -> configs/*.cfg   (1 single / 2 hybrid: fast then slow)
      * Ramulator 2.0      -> configs/r2/*.yaml (1 single / 2 hybrid: fast then slow)
    """
    configs = repo_root / "configs"
    if mode.ramulator1:
        fast, slow = configs / "HBM-config.cfg", configs / "DDR4-config.cfg"
        return [fast, slow] if mode.hybrid else [slow]
    if mode.ramulator2:
        fast, slow = configs / "r2" / "HBM.yaml", configs / "r2" / "DDR4.yaml"
        return [fast, slow] if mode.hybrid else [slow]
    return []  # ChampSim-only


@dataclass
class RunResult:
    returncode: int
    stdout: str
    stderr: str
    statistics_path: Path  # expected location; may not exist if the run failed
    argv: list[str]

    @property
    def completed(self) -> bool:
        return COMPLETION_MARKER in self.stdout

    @property
    def statistics_written(self) -> bool:
        return self.statistics_path.is_file() and self.statistics_path.stat().st_size > 0


def statistics_filename_for(trace: Path) -> str:
    """The binary names its stats file ``<trace-basename>.statistics`` (see
    DATA_OUTPUT::output_file_initialization in source/ProjectConfiguration.cc)."""
    return trace.name + ".statistics"


def run_simulation(
    binary: Path,
    config_files: list[Path],
    traces: list[Path],
    warmup: int,
    simulation: int,
    workdir: Path,
    timeout: float = 1800.0,
) -> RunResult:
    """Run the simulator with absolute paths, capturing stdout/stderr.

    ``workdir`` becomes the process CWD so the ``.statistics`` / ``.trace`` output
    files land there instead of polluting the repository. All other paths are made
    absolute so the choice of CWD does not affect argument resolution.
    """
    argv = [
        os.fspath(binary.resolve()),
        "--warmup-instructions", str(warmup),
        "--simulation-instructions", str(simulation),
        *[os.fspath(c.resolve()) for c in config_files],
        *[os.fspath(t.resolve()) for t in traces],
    ]
    workdir.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        argv,
        cwd=os.fspath(workdir),
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    # The stats file is named after the first trace's basename.
    statistics_path = workdir / statistics_filename_for(traces[0])
    return RunResult(
        returncode=proc.returncode,
        stdout=proc.stdout,
        stderr=proc.stderr,
        statistics_path=statistics_path,
        argv=argv,
    )


_IPC_RE = re.compile(r"cumulative IPC:\s*([0-9]*\.?[0-9]+)")
_L1D_RE = re.compile(r"cpu0->cpu0_L1D TOTAL\s+ACCESS:\s*([0-9]+)")


@dataclass
class ParsedStatistics:
    completed: bool
    last_cumulative_ipc: float | None
    l1d_total_access: int | None
    has_dram_section: bool


def parse_statistics(text: str) -> ParsedStatistics:
    """Pull a handful of sanity signals out of stdout or a ``.statistics`` file."""
    ipc_matches = _IPC_RE.findall(text)
    l1d_match = _L1D_RE.search(text)
    return ParsedStatistics(
        completed=COMPLETION_MARKER in text,
        last_cumulative_ipc=float(ipc_matches[-1]) if ipc_matches else None,
        l1d_total_access=int(l1d_match.group(1)) if l1d_match else None,
        has_dram_section="DRAM Statistics" in text,
    )

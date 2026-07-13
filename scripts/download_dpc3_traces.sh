#!/bin/bash

# Usage: ./download_dpc3_traces.sh [trace-list-file]
#
# Downloads the DPC-3 traces listed in the given file (one trace filename per
# line; blank lines and lines starting with '#' are ignored). The list file
# defaults to dpc3_max_simpoint.txt (the max-simpoint set, ~20GB). Traces are
# fetched with `wget -c` (resumable) into the dpc3_traces directory inside
# ChampSim-Ramulator/, which is where the simulator expects them.

# Resolve the script's own directory so the paths below do not depend on $PWD.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Trace directory: dpc3_traces inside ChampSim-Ramulator/ (scripts/ is one level
# deeper, hence .. from SCRIPT_DIR).
TRACE_DIR="$SCRIPT_DIR/../dpc3_traces"

# Select the trace-list file (default: dpc3_max_simpoint.txt). If the given
# name is not found as-is, fall back to the copy next to this script so the
# script works whether run from scripts/ or from the project root.
LIST_FILE="${1:-dpc3_max_simpoint.txt}"
if [ ! -f "$LIST_FILE" ] && [ -f "$SCRIPT_DIR/$LIST_FILE" ]; then
    LIST_FILE="$SCRIPT_DIR/$LIST_FILE"
fi
if [ ! -f "$LIST_FILE" ]; then
    echo "Trace list file not found: $LIST_FILE" >&2
    exit 1
fi

mkdir -p "$TRACE_DIR"
while read LINE
do
    # Skip blank lines and comments.
    [ -z "$LINE" ] && continue
    case "$LINE" in \#*) continue ;; esac
    wget -P "$TRACE_DIR" -c https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/$LINE
done < "$LIST_FILE"

#!/bin/bash
# Builds (if needed) and starts Cubiomes Viewer with Tectonic support.
#
#   ./run-tectonic.sh [viewer options]        JOBS=<n> sets the number of build jobs (default 3)
#
# Settings and sessions of this build live in ~/.config/cubiomes-viewer-tectonic/, separate from a regular
# Cubiomes Viewer and from the Flatpak.
cd "$(dirname "$0")" || exit 1
JOBS=${JOBS:-3}
if [ ! -f cubiomes/tectonic.h ]; then
  git submodule update --init cubiomes || { echo "cannot fetch the cubiomes submodule" >&2; exit 1; }
fi
for tool in qmake cmake make; do
  command -v $tool >/dev/null || { echo "$tool not found (needs Qt 5 development files, cmake and make)" >&2; exit 1; }
done
mkdir -p build && cd build || exit 1
if [ ! -f Makefile ] || [ ../cubiomes-viewer.pro -nt Makefile ]; then
  qmake ../cubiomes-viewer.pro || exit 1
fi
if ! make -j"$JOBS" > build.log 2>&1; then
  echo "build failed, see build/build.log" >&2; tail -20 build.log >&2; exit 1
fi
exec ./cubiomes-viewer "$@"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build-tracy"
EXE="$BUILD/AnimationRasteriser"

echo "==> Configuring with Tracy ON..."
cmake -B "$BUILD" -S "$ROOT" -DUSE_TRACY=ON

echo "==> Building..."
cmake --build "$BUILD"

echo "==> Running (start tracy-profiler first)..."
"$EXE"

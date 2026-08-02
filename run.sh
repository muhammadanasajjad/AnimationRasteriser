#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

echo "==> Configuring..."
cmake -B "$BUILD" -S "$ROOT"

echo "==> Building..."
cmake --build "$BUILD"

echo "==> Running..."
"$BUILD/AnimationRasteriser"

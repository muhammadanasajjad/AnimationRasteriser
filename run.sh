#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "==> Configuring..."
cmake -B "$ROOT/build" -S "$ROOT"

echo "==> Building..."
cmake --build "$ROOT/build"

echo "==> Running..."
"$ROOT/build/AnimationRasteriser"

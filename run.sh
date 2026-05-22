#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

#echo "==> Installing dependencies..."
#sudo apt update && sudo apt install -y libglfw3-dev libglm-dev

echo "==> Configuring..."
cmake -B "$ROOT/build" -S "$ROOT"

echo "==> Building..."
cmake --build "$ROOT/build"

echo "==> Running..."
"$ROOT/build/AnimationRasteriser"

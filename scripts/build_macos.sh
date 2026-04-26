#!/bin/bash
set -e

# プロジェクトのルートディレクトリに移動
cd "$(dirname "$0")/.."

echo "--- Configuring with CMake ---"
cmake -S . -B build

echo "--- Building M88M ---"
# CPUコア数に合わせて並列ビルド
cmake --build build -j$(sysctl -n hw.ncpu)

echo "--- Build Complete ---"
echo "Executable: ./build/m88"

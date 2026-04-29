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
if [ "$(uname)" == "Darwin" ]; then
    echo "Bundle: ./build/M88M.app"
    echo "Executable: ./build/M88M.app/Contents/MacOS/M88M"
else
    echo "Executable: ./build/m88"
fi

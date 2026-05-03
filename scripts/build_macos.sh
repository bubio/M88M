#!/bin/bash
set -e

# プロジェクトのルートディレクトリに移動
cd "$(dirname "$0")/.."

echo "--- Configuring with CMake ---"
if [ "$MACOS_UNIVERSAL" == "1" ]; then
    cmake -S . -B build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"
else
    cmake -S . -B build
fi

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

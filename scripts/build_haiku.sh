#!/bin/sh
set -e

# M88M Build Script for Haiku

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

printf "%b\n" "${GREEN}=== M88M Haiku Build Script ===${NC}"

if ! command -v cmake > /dev/null 2>&1; then
    printf "%b\n" "${RED}Error: cmake is not installed.${NC}"
    echo "Please install it with: pkgman install cmake"
    exit 1
fi

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

BUILD_DIR="build"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 2)"

printf "%b\n" "${YELLOW}Configuring project...${NC}"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo

printf "%b\n" "${YELLOW}Building...${NC}"
cmake --build "$BUILD_DIR" -j"$JOBS"

printf "%b\n" "${GREEN}Build complete!${NC}"
printf "%b\n" "The executable is located at: ${YELLOW}$PROJECT_ROOT/$BUILD_DIR/m88m${NC}"
echo ""
echo "Make sure these packages are installed first:"
echo "pkgman install cmake git pkgconfig libiconv_devel libsdl2_devel"
echo "raylib 6.0 is built from source by CMake."

#!/bin/bash
set -e

# M88M Build Script for Linux
# This script configures and builds the project using CMake.

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== M88M Linux Build Script ===${NC}"

print_dependency_message() {
    echo "Please install the Linux build dependencies listed in README.md:"
    echo ""
    echo "Arch-based (pacman):"
    echo "  sudo pacman -S --needed base-devel cmake git libx11 libxcursor libxinerama libxrandr libxi mesa alsa-lib gtk3"
    echo ""
    echo "Fedora / RHEL-based (dnf):"
    echo "  sudo dnf install -y gcc-c++ make cmake git libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel mesa-libGL-devel alsa-lib-devel gtk3-devel"
    echo ""
    echo "Debian-based (apt):"
    echo "  sudo apt-get install build-essential cmake git libasound2-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev libgl1-mesa-dev libgtk-3-dev"
}

# Check for dependencies (simplified check)
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is not installed.${NC}"
    print_dependency_message
    exit 1
fi

# Determine project root (script is in scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Build directory
BUILD_DIR="build"

echo -e "${YELLOW}Configuring project...${NC}"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo

echo -e "${YELLOW}Building...${NC}"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo -e "${GREEN}Build complete!${NC}"
echo -e "The executable is located at: ${YELLOW}$PROJECT_ROOT/$BUILD_DIR/m88m${NC}"
echo ""
echo "Note: Make sure the required Linux build dependencies are installed:"
print_dependency_message

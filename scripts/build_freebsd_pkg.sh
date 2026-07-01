#!/bin/sh
set -eu

# M88M Build + Package Script for FreeBSD

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

printf "%b\n" "${GREEN}=== M88M FreeBSD Package Script ===${NC}"

for tool in cmake pkg; do
    if ! command -v "$tool" > /dev/null 2>&1; then
        printf "%b\n" "${RED}Error: $tool is not installed.${NC}"
        exit 1
    fi
done

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
PKG_STAGE_DIR="$BUILD_DIR/pkgstage"
STAGE_ROOT="$PKG_STAGE_DIR/root"
META_DIR="$PKG_STAGE_DIR/meta"
PLIST_FILE="$PKG_STAGE_DIR/plist"
DIST_DIR="$PROJECT_ROOT/dist"

cd "$PROJECT_ROOT"

sh "$SCRIPT_DIR/build_freebsd.sh"

printf "%b\n" "${YELLOW}Staging install tree...${NC}"
rm -rf "$PKG_STAGE_DIR"
mkdir -p "$STAGE_ROOT" "$META_DIR" "$DIST_DIR"
DESTDIR="$STAGE_ROOT" cmake --install "$BUILD_DIR" --prefix /usr/local

VERSION="$(cmake -LA -N "$BUILD_DIR" 2>/dev/null | sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' | head -n 1)"
if [ -z "$VERSION" ]; then
    VERSION="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9][0-9.]*\).*/\1/p' "$PROJECT_ROOT/CMakeLists.txt" | head -n 1)"
fi
if [ -z "$VERSION" ]; then
    printf "%b\n" "${RED}Error: failed to determine project version.${NC}"
    exit 1
fi

find "$STAGE_ROOT" \( -type f -o -type l \) | sed "s#^$STAGE_ROOT##" | LC_ALL=C sort > "$PLIST_FILE"
if [ ! -s "$PLIST_FILE" ]; then
    printf "%b\n" "${RED}Error: staged file list is empty.${NC}"
    exit 1
fi

cat > "$META_DIR/+MANIFEST" <<EOF
name: m88m
version: "$VERSION"
origin: emulators/m88m
comment: "PC-8801 emulator (M88 kai) - raylib frontend"
maintainer: "bubio@users.noreply.github.com"
www: "https://github.com/bubio/M88M"
prefix: /usr/local
licenses: [ "Custom" ]
categories: [ "emulators", "games" ]
desc: <<EOD
M88M is a PC-8801 emulator based on M88 with a raylib frontend.
EOD
EOF

printf "%b\n" "${YELLOW}Creating pkg artifact...${NC}"
find "$DIST_DIR" -type f -name 'm88m-*.pkg' -delete
pkg create -M "$META_DIR/+MANIFEST" -r "$STAGE_ROOT" -p "$PLIST_FILE" -o "$DIST_DIR"

PKG_FILE="$(find "$DIST_DIR" -type f -name 'm88m-*.pkg' | LC_ALL=C sort | tail -n 1)"
if [ -z "$PKG_FILE" ]; then
    printf "%b\n" "${RED}Error: pkg artifact was not created.${NC}"
    exit 1
fi

# pkg names the artifact "m88m-<version>.pkg" with no architecture; rename it
# to include the arch (e.g. m88m-1.2.0-FreeBSD-amd64.pkg) so release assets
# are distinguishable per platform.
ARCH="$(uname -m)"
ARCH_PKG_FILE="$DIST_DIR/m88m-$VERSION-FreeBSD-$ARCH.pkg"
if [ "$PKG_FILE" != "$ARCH_PKG_FILE" ]; then
    mv "$PKG_FILE" "$ARCH_PKG_FILE"
    PKG_FILE="$ARCH_PKG_FILE"
fi

printf "%b\n" "${GREEN}Package complete!${NC}"
printf "%b\n" "The package is located at: ${YELLOW}$PKG_FILE${NC}"

#!/bin/sh
set -e

APP_PATH="${1:-build/m88m}"
OUT_PATH="${2:-dist/m88m-1.2.0-haiku-x64.hpkg}"
VERSION="${3:-1.2.0}"
REVISION="${4:-1}"

if [ ! -f "$APP_PATH" ]; then
    echo "Application executable not found: $APP_PATH" >&2
    exit 1
fi

if ! command -v package >/dev/null 2>&1; then
    echo "Haiku package command not found." >&2
    exit 1
fi

OUT_DIR="$(dirname "$OUT_PATH")"
ROOT_DIR="$OUT_DIR/hpkg-root"
PACKAGE_VERSION="$VERSION-$REVISION"

rm -rf "$ROOT_DIR"
mkdir -p "$ROOT_DIR/apps/M88M"
mkdir -p "$ROOT_DIR/data/licenses/m88m"
mkdir -p "$OUT_DIR"

cp "$APP_PATH" "$ROOT_DIR/apps/M88M/M88M"
chmod 755 "$ROOT_DIR/apps/M88M/M88M"

if [ -f LICENSE ]; then
    cp LICENSE "$ROOT_DIR/data/licenses/m88m/MIT"
fi
if [ -f assets/NOTICE.md ]; then
    cp assets/NOTICE.md "$ROOT_DIR/data/licenses/m88m/NOTICE.md"
fi
if [ -f assets/OFL.txt ]; then
    cp assets/OFL.txt "$ROOT_DIR/data/licenses/m88m/OFL-1.1"
fi

if [ -x scripts/prepare_haiku_app.sh ]; then
    scripts/prepare_haiku_app.sh "$ROOT_DIR/apps/M88M/M88M" assets/AppIcon.png
fi

cat > "$ROOT_DIR/.PackageInfo" <<EOF
name m88m
version $PACKAGE_VERSION
architecture x86_64
summary "PC-8801 emulator"
description "M88M is a PC-8801 emulator with a raylib frontend."
packager "bubio"
vendor "bubio"
licenses {
    "MIT"
    "OFL-1.1"
}
copyrights {
    "M88M contributors"
}
provides {
    m88m = $PACKAGE_VERSION
    app:M88M = $PACKAGE_VERSION
}
requires {
    haiku >= r1~beta5
    lib:libbe
    lib:libroot
    lib:libmedia
    lib:libtracker
}
EOF

rm -f "$OUT_PATH"
package create -C "$ROOT_DIR" "$OUT_PATH"
echo "Created Haiku package: $OUT_PATH"

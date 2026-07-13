#!/bin/sh
set -e

APP_PATH="${1:-}"
ICON_PATH="${2:-}"
APP_SIGNATURE="application/x-vnd.bubio-m88m"

if [ -z "$APP_PATH" ] || [ ! -f "$APP_PATH" ]; then
    echo "Usage: $0 path/to/m88m [path/to/AppIcon.hvif]" >&2
    exit 1
fi

if command -v settype >/dev/null 2>&1; then
    settype -t "$APP_SIGNATURE" "$APP_PATH" || true
elif command -v addattr >/dev/null 2>&1; then
    addattr -t mime BEOS:TYPE "$APP_SIGNATURE" "$APP_PATH" || true
fi

if command -v addattr >/dev/null 2>&1; then
    addattr -t string BEOS:APP_SIG "$APP_SIGNATURE" "$APP_PATH" || true
fi

if [ -n "$ICON_PATH" ] && [ -f "$ICON_PATH" ]; then
    if command -v seticon >/dev/null 2>&1; then
        seticon "$ICON_PATH" "$APP_PATH"
    elif command -v c++ >/dev/null 2>&1; then
        HELPER="/tmp/m88m_set_hvif_icon_$$"
        SRC="$HELPER.cpp"
        cat > "$SRC" <<'CPP'
#include <Node.h>
#include <SupportDefs.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s app icon.hvif\n", argv[0]);
        return 2;
    }

    std::ifstream in(argv[2], std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.empty()) {
        std::fprintf(stderr, "empty or unreadable icon: %s\n", argv[2]);
        return 1;
    }

    BNode node(argv[1]);
    status_t status = node.InitCheck();
    if (status != B_OK) {
        std::fprintf(stderr, "BNode init failed: %ld\n", (long)status);
        return 1;
    }

    ssize_t written = node.WriteAttr("BEOS:ICON", 'VICN', 0, data.data(), data.size());
    if (written != (ssize_t)data.size()) {
        std::fprintf(stderr, "WriteAttr BEOS:ICON failed: %ld\n", (long)written);
        return 1;
    }
    return 0;
}
CPP
        c++ "$SRC" -o "$HELPER" -lbe
        "$HELPER" "$APP_PATH" "$ICON_PATH"
        rm -f "$SRC" "$HELPER"
    else
        echo "seticon and c++ are not available; cannot attach Haiku icon." >&2
        exit 1
    fi
fi

if command -v mimeset >/dev/null 2>&1; then
    mimeset -f "$APP_PATH" || true
fi

echo "Prepared Haiku app metadata for $APP_PATH"

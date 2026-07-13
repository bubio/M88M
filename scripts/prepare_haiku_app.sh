#!/bin/sh
set -e

APP_PATH="${1:-}"
ICON_PATH="${2:-}"
APP_SIGNATURE="application/x-vnd.bubio-m88m"

if [ -z "$APP_PATH" ] || [ ! -f "$APP_PATH" ]; then
    echo "Usage: $0 path/to/m88m [path/to/AppIcon.png]" >&2
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

if [ -n "$ICON_PATH" ] && [ -f "$ICON_PATH" ] && command -v seticon >/dev/null 2>&1; then
    seticon "$ICON_PATH" "$APP_PATH" || true
fi

if command -v mimeset >/dev/null 2>&1; then
    mimeset -f "$APP_PATH" || true
fi

echo "Prepared Haiku app metadata for $APP_PATH"

#!/usr/bin/env bash
# Syncs version string from puzzle-plus-plus/VERSION file to CMakeLists.txt.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="$SCRIPT_DIR/VERSION"

if [ ! -f "$VERSION_FILE" ]; then
    echo "⚠️ Submodule VERSION file not found at $VERSION_FILE"
    exit 0
fi

VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")

if [ -z "$VERSION" ]; then
    echo "⚠️ Submodule VERSION file is empty"
    exit 0
fi

CMAKE_FILE="$SCRIPT_DIR/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
    sed -i -E "s/set\(p\+\+_VERSION \"[^\"]+\"\)/set(p++_VERSION \"$VERSION\")/" "$CMAKE_FILE"
    echo "  → Updated $CMAKE_FILE to version $VERSION"
fi

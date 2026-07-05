#!/bin/bash
# Builds a real macOS installer (.pkg) for Life Key, the same kind of
# installer commercial plugins ship with. Run this AFTER you've already
# built the plugin with cmake --build (see README.md steps 1-8).

set -e

IDENTIFIER="com.jayskilly.lifekey"
VERSION="1.0.0"
BUILD_DIR="build/ScaleLock_artefacts/Release"
STAGE_DIR="packaging/stage"
OUT_PKG="Life Key Installer.pkg"

if [ ! -d "$BUILD_DIR/VST3/Life Key.vst3" ]; then
    echo "Error: couldn't find the built plugin at $BUILD_DIR/VST3/Life Key.vst3"
    echo "Make sure you've run the build steps in README.md first."
    exit 1
fi

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGE_DIR/Library/Audio/Plug-Ins/Components"

cp -R "$BUILD_DIR/VST3/Life Key.vst3" "$STAGE_DIR/Library/Audio/Plug-Ins/VST3/"
cp -R "$BUILD_DIR/AU/Life Key.component" "$STAGE_DIR/Library/Audio/Plug-Ins/Components/"

pkgbuild --root "$STAGE_DIR" \
         --identifier "$IDENTIFIER" \
         --version "$VERSION" \
         --install-location "/" \
         "$OUT_PKG"

rm -rf "$STAGE_DIR"

echo ""
echo "Done! Created: $OUT_PKG"
echo "Double-click it - it'll show a normal macOS installer window,"
echo "ask for your password, and install Life Key for every DAW on your Mac."

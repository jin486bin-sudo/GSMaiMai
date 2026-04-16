#!/bin/bash
# GSMaiMai VST3/AU Installer
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
VST3_SRC="$DIR/GSMaiMai.vst3"
AU_SRC="$DIR/GSMaiMai.component"
VST3_DST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DST="$HOME/Library/Audio/Plug-Ins/Components"

echo "======================================"
echo "  GSMaiMai Tape Plugin Installer"
echo "======================================"
echo ""

mkdir -p "$VST3_DST" "$AU_DST"

if [ -d "$VST3_SRC" ]; then
    echo "[1/3] Installing VST3..."
    rm -rf "$VST3_DST/GSMaiMai.vst3"
    cp -R "$VST3_SRC" "$VST3_DST/"
    xattr -dr com.apple.quarantine "$VST3_DST/GSMaiMai.vst3" 2>/dev/null || true
    codesign --force --deep --sign - "$VST3_DST/GSMaiMai.vst3" 2>/dev/null || true
    echo "      OK → $VST3_DST/GSMaiMai.vst3"
fi

if [ -d "$AU_SRC" ]; then
    echo "[2/3] Installing AU..."
    rm -rf "$AU_DST/GSMaiMai.component"
    cp -R "$AU_SRC" "$AU_DST/"
    xattr -dr com.apple.quarantine "$AU_DST/GSMaiMai.component" 2>/dev/null || true
    codesign --force --deep --sign - "$AU_DST/GSMaiMai.component" 2>/dev/null || true
    echo "      OK → $AU_DST/GSMaiMai.component"
fi

echo "[3/3] Resetting AU cache..."
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache 2>/dev/null || true

echo ""
echo "Installation complete."
echo "Restart your DAW (Cubase) to load the plugin."
echo ""
read -p "Press Enter to close..."

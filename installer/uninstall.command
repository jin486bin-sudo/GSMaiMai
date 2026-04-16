#!/bin/bash
echo "Uninstalling GSMaiMai..."
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/GSMaiMai.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/GSMaiMai.component"
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache 2>/dev/null || true
echo "Done."
read -p "Press Enter to close..."

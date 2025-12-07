#!/bin/bash

# Install DB303 VST plugin to system directory

set -e  # Exit on error

PLUGIN_DIR=~/Library/Audio/Plug-Ins/VST
SOURCE_PLUGIN=build/AcidSeq303_artefacts/Release/VST3/AcidSeq-303.vst3

cd build && cmake --build . --target AcidSeq303_All -j4
cd ..

# Remove quarantine attribute from source (if present)
echo "Removing quarantine attribute from source..."
xattr -rd com.apple.quarantine "$SOURCE_PLUGIN" 2>/dev/null || true

# Create plugin directory if it doesn't exist
echo "Creating VST plugin directory..."
mkdir -p "$PLUGIN_DIR"

# Copy plugin to system directory
echo "Installing AcidSeq303.vst3 to $PLUGIN_DIR..."
cp -r "$SOURCE_PLUGIN" "$PLUGIN_DIR/"

echo "✓ AcidSeq303.vst3 successfully installed to $PLUGIN_DIR"

#!/bin/bash
# Clean all build artifacts

echo "Cleaning build artifacts..."

# Object files
rm -f source_c/*.o
rm -f source_c/**/*.o

# Native executables
rm -f alan_parsons_c alan_parsons

# Test binaries and debug symbols
rm -f tests/test_game tests/test_playthrough tests/test_pool tests/test_render tests/test_vec2
rm -f tests/test_bridge tests/test_bridge_opaque tests/test_play_feel tests/test_ai_selfplay tests/test_audio
rm -rf tests/*.dSYM

# WASM build
rm -rf wasm_build/

echo "Done."

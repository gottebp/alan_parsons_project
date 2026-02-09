#!/bin/bash
# Clean all build artifacts

echo "Cleaning build artifacts..."

# Object files
rm -f source_c/*.o

# Native executables
rm -f alan_parsons_c alan_parsons

# WASM build
rm -rf wasm_build/

echo "Done."

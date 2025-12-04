#!/bin/bash
# Build script for The Alan Parsons Project - WebAssembly version
# This script builds the game for browser deployment with compression

set -e  # Exit on error

echo "======================================================================"
echo "  The Alan Parsons Project - WebAssembly Build Script"
echo "======================================================================"
echo ""

# Check if emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: emscripten not found!"
    echo ""
    echo "Please install emscripten first:"
    echo "  brew install emscripten"
    echo ""
    exit 1
fi

# Show emscripten version
echo "✓ Found emscripten:"
emcc --version | head -1
echo ""

# Check if data files exist
if [ ! -d "data" ] || [ ! -d "sound" ]; then
    echo "❌ Error: Missing data/ or sound/ directories"
    echo "Please ensure game assets are in the current directory"
    exit 1
fi

echo "✓ Found game assets (data/ and sound/)"
echo ""

# Clean previous build
echo "🧹 Cleaning previous build..."
make -f Makefile.emscripten clean > /dev/null 2>&1

# Build
echo "🔨 Building WebAssembly version..."
echo "   (This may take a few minutes...)"
echo ""

# Run make and capture output
if make -f Makefile.emscripten 2>&1 | tee /tmp/wasm_build.log | grep -E "(Creating|Compiling|Compressing|Patching|Build complete|warning:)"; then
    echo ""
    echo "======================================================================"
    echo "✅ Build successful!"
    echo "======================================================================"
    echo ""

    # Show build statistics
    echo "📊 Build Statistics:"
    echo "-------------------"
    ls -lh wasm_build/ | grep -E "game\.(data\.gz|wasm|js|html)" | awk '{printf "  %-20s %8s\n", $9, $5}'
    echo ""
    echo "  Total size:        $(du -sh wasm_build/ | awk '{print $1}')"
    echo ""

    # Show what to do next
    echo "🚀 To run the game:"
    echo "-------------------"
    echo "  1. Start a local web server:"
    echo "     make -f Makefile.emscripten serve"
    echo ""
    echo "  2. Open in browser:"
    echo "     http://localhost:8000/wasm_build/game.html"
    echo ""
    echo "  Or use any web server of your choice to serve the wasm_build/ directory"
    echo ""
    echo "📱 Mobile Access:"
    echo "-------------------"
    echo "  Find your local IP: ifconfig | grep 'inet ' | grep -v 127.0.0.1"
    echo "  Then access from mobile: http://[your-ip]:8000/wasm_build/game.html"
    echo ""
    echo "💾 Files ready for deployment in: wasm_build/"
    echo ""

else
    echo ""
    echo "======================================================================"
    echo "❌ Build failed!"
    echo "======================================================================"
    echo ""
    echo "Check the error messages above."
    echo "Full log saved to: /tmp/wasm_build.log"
    echo ""
    exit 1
fi

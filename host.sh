#!/bin/bash
# Host the WASM build locally for testing

PORT="${1:-8000}"

if [ ! -d "wasm_build" ]; then
    echo "Error: wasm_build directory not found. Run ./build.sh first."
    exit 1
fi

if [ ! -f "wasm_build/game.html" ]; then
    echo "Error: wasm_build/game.html not found. Run ./build.sh first."
    exit 1
fi

# Kill any existing server on this port
lsof -ti:$PORT | xargs kill -9 2>/dev/null || true

echo "Starting server at http://localhost:$PORT/game.html"
echo "Press Ctrl+C to stop"
cd wasm_build && python3 -m http.server $PORT

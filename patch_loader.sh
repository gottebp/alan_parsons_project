#!/bin/bash
# Patch game.data.js and game.js to load compressed .gz file and decompress it

DATAJS="wasm_build/game.data.js"
GAMEJS="wasm_build/game.js"

echo "Patching $DATAJS to use gzip compression..."

# Change both package names to .gz
sed -i.bak "s/var PACKAGE_NAME = 'wasm_build\/game.data';/var PACKAGE_NAME = 'wasm_build\/game.data.gz';/" "$DATAJS"
sed -i.bak2 "s/var REMOTE_PACKAGE_BASE = 'game.data';/var REMOTE_PACKAGE_BASE = 'game.data.gz';/" "$DATAJS"
sed -i.bak3 "s/datafile_wasm_build\/game.data/datafile_wasm_build\/game.data.gz/g" "$DATAJS"

# Decompression code block
DECOMPRESS_CODE='        // Decompress gzip data using DecompressionStream
        console.log("[fetchRemotePackage] Decompressing, size:", packageData.byteLength);
        try {
          const ds = new DecompressionStream("gzip");
          const decompressedStream = new Response(packageData.buffer).body.pipeThrough(ds);
          const decompressed = await new Response(decompressedStream).arrayBuffer();
          console.log("[fetchRemotePackage] Decompressed size:", decompressed.byteLength);
          return decompressed;
        } catch (e) {
          console.error("[fetchRemotePackage] Decompression failed:", e);
          throw e;
        }'

# Add decompression to game.data.js
cat > /tmp/decompress_patch.txt << 'EOF'
        // Decompress gzip data using DecompressionStream
        console.log("[fetchRemotePackage] Decompressing, size:", packageData.byteLength);
        try {
          const ds = new DecompressionStream("gzip");
          const decompressedStream = new Response(packageData.buffer).body.pipeThrough(ds);
          const decompressed = await new Response(decompressedStream).arrayBuffer();
          console.log("[fetchRemotePackage] Decompressed size:", decompressed.byteLength);
          return decompressed;
        } catch (e) {
          console.error("[fetchRemotePackage] Decompression failed:", e);
          throw e;
        }
EOF

sed -i.bak2 '/return packageData.buffer;/{
r /tmp/decompress_patch.txt
d
}' "$DATAJS"

echo "Patching game.js to use .gz files and add decompression..."
# Change filenames to .gz
sed -i.bak4 's/game\.data"/game.data.gz"/g' "$GAMEJS"
sed -i.bak5 "s/game\.data'/game.data.gz'/g" "$GAMEJS"

# Add decompression to game.js as well
sed -i.bak6 '/return packageData.buffer;/{
r /tmp/decompress_patch.txt
d
}' "$GAMEJS"

echo "Patched successfully"
rm -f "$DATAJS.bak" "$DATAJS.bak2" "$DATAJS.bak3" "$DATAJS.bak4" "$GAMEJS.bak4" "$GAMEJS.bak5" "$GAMEJS.bak6" /tmp/decompress_patch.txt

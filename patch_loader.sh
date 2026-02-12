#!/bin/bash
# Patch game.data.js to load compressed .gz file and decompress it
# MUST be run BEFORE emcc compilation (game.data.js is embedded via --pre-js)

DATAJS="wasm_build/game.data.js"

echo "Patching $DATAJS to use gzip compression..."

# Change both package names to .gz
sed -i.bak "s/var PACKAGE_NAME = 'wasm_build\/game.data';/var PACKAGE_NAME = 'wasm_build\/game.data.gz';/" "$DATAJS"
sed -i.bak2 "s/var REMOTE_PACKAGE_BASE = 'game.data';/var REMOTE_PACKAGE_BASE = 'game.data.gz';/" "$DATAJS"
sed -i.bak3 "s/datafile_wasm_build\/game.data/datafile_wasm_build\/game.data.gz/g" "$DATAJS"

# Add decompression before the return statement in fetchRemotePackage
cat > /tmp/decompress_patch.txt << 'EOF'
        // Check for gzip magic bytes (0x1f 0x8b) before decompressing
        if (packageData[0] === 0x1f && packageData[1] === 0x8b) {
          console.log("[fetchRemotePackage] Gzip detected, decompressing", packageData.byteLength, "bytes...");
          try {
            const ds = new DecompressionStream("gzip");
            const decompressedStream = new Response(packageData.buffer).body.pipeThrough(ds);
            const decompressed = await new Response(decompressedStream).arrayBuffer();
            console.log("[fetchRemotePackage] Decompressed:", decompressed.byteLength, "bytes");
            return decompressed;
          } catch (e) {
            console.error("[fetchRemotePackage] Decompression failed:", e);
            throw e;
          }
        } else {
          console.log("[fetchRemotePackage] Data already decompressed:", packageData.byteLength, "bytes");
          return packageData.buffer;
        }
EOF

sed -i.bak4 '/return packageData.buffer;/{
r /tmp/decompress_patch.txt
d
}' "$DATAJS"

echo "Patched successfully"
rm -f "$DATAJS".bak "$DATAJS".bak2 "$DATAJS".bak3 "$DATAJS".bak4 /tmp/decompress_patch.txt

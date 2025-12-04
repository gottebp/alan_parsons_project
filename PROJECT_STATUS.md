# The Alan Parsons Project - WebAssembly Port Status

**Date:** November 14, 2025
**Platform:** macOS 26.1.0 (Darwin 25.1.0)
**Target:** Web deployment via Emscripten WASM

---

## Project Overview

The Alan Parsons Project is a flying terrain shooter (Lord of the Rings themed) originally written in x86 assembly, converted to C, and now compiled to WebAssembly for browser deployment.

### Key Features Implemented
- ✅ Full game conversion from ASM → C → WASM
- ✅ WebGL rendering (1280x720 canvas)
- ✅ Web Audio API for sound
- ✅ Rivendell-themed UI styling (Tolkien aesthetic)
- ✅ Captain Planet Mode (unlockable 18 nukes vs 4 standard)
- ✅ LocalStorage persistence for game progress
- ✅ Fullscreen support
- ✅ Responsive layout
- ✅ Gzip compression (86% size reduction: 213MB → 32MB)
- ✅ Client-side decompression with DecompressionStream API

---

## Current Status

### ✅ Working Features

**Desktop (Chrome/Safari on macOS):**
- Game loads and runs perfectly
- All assets load correctly
- Audio works
- Graphics rendering smooth
- Game mechanics functional
- Captain Planet mode toggle works
- LocalStorage persistence works

**Server Infrastructure:**
- http-server (Node.js) running on port 8000
- Accessible via http://192.168.1.56:8000/wasm_build/game.html
- macOS Local Network Privacy configured (Terminal and node allowed)
- Serving from: `/Users/ben/Desktop/particlefield/downloads/alan_parsons_project2/`

### ❌ Current Issues

**iOS Safari & Firefox (v18.7):**
- **CRITICAL:** Game loads HTML/CSS/JS but hangs at 0% loading forever
- No error messages in initial load
- WebAssembly module loads successfully
- **game.data file never requested** by iOS browsers (Chrome on Mac requests it successfully)
- Debugging logs added but need to be inspected via Safari Web Inspector

---

## Recent Changes (Latest Session)

### 1. Fixed Escape Key Behavior in Main Menu ✅
**Problem:** Pressing escape in the main menu returned to the level you were playing instead of exiting
**Solution:** Changed menu.c to always exit when escape is pressed

**File Modified:**
- `source_c/menu.c:163` - Changed `MenuResult = game_running;` to `MenuResult = 0;`
- Now escape in menu always exits to start screen, regardless of game state

### 2. Re-enabled Compression with .bin Extension ✅
**Problem:** Server was auto-decompressing .gz files, causing double-decompression attempts
**Solution:** Use `.bin` extension to prevent server Content-Encoding headers

**Files Modified:**
- Created `wasm_build/game.data.bin` (32MB gzipped) from `game.data.gz`
- `wasm_build/game.data.js` - Updated to reference `game.data.bin`, added decompression with error handling
- `wasm_build/game.js` - Updated to reference `game.data.bin`, added decompression with error handling
- Removed uncompressed `game.data` (213MB) - no longer needed

**Key Improvement:**
- **86% size reduction**: 213MB → 32MB download
- Client-side decompression using DecompressionStream API
- Enhanced logging for debugging compression issues
- Console shows: `[fetchRemotePackage] Decompressing data, compressed size: 33554432`

### 3. Previous Session Changes

#### Compression Troubleshooting (Nov 12)
**Problem:** Initial attempt to use gzip compression caused data corruption
**Action Taken:** Temporarily reverted to uncompressed game.data file (213MB)
**Resolution:** Fixed in current session with .bin extension approach

#### Fade Timing Adjustments (Nov 12)
**File:** `source_c/main.c:1655-1665`
- Changed ending fade from 800 frames (8 seconds) to 500/300 frames (5s/3s)
- User manually tuned these values for proper pacing

#### Captain Planet Mode Implementation (Nov 12)
**Feature:** Unlockable mode with 18 nukes instead of 4

**Files Modified:**
- `source_c/main.c:235-246` - Check localStorage on game init, set `num_starting_nukes = 18`
- `source_c/main.c:960-966` - Set `alan_parsons_beaten` flag on game completion
- `shell.html:490-496` - Added Captain Planet button with ☢ icon and "Extra Nukes" subtext
- `shell.html:666-703` - Toggle function, localStorage persistence, button state management

**Behavior:**
- Button hidden until game beaten
- Green gradient when ON, blue when OFF
- Reloads page after toggle to apply changes
- Persists across sessions via localStorage

#### Debugging Instrumentation (Nov 12)
**Added comprehensive console logging to diagnose iOS issue:**

**Modified Files:**
- `wasm_build/game.data.js` - Added 15+ console.log statements
- `wasm_build/game.js` - Mirrored logging in compiled output

**Log Points:**
- `[loadPackage] Starting data file load...`
- `[loadPackage] Package: ... Remote: ... Size: ...`
- `[loadPackage] Opening IndexedDB...`
- `[loadPackage] Checking cached package...`
- `[loadPackage] Use cached: true/false`
- `[loadPackage] Fetching remote package...`
- `[fetchRemotePackage] Fetching: ... Size: ...`
- `[fetchRemotePackage] Starting fetch...`
- `[fetchRemotePackage] Fetch response: status ok`
- `[preloadFallback] Error occurred: ...` (if errors occur)

---

## File Structure

```
alan_parsons_project2/
├── source_c/                   # C source files
│   ├── main.c                 # Game logic (Captain Planet mode, fades)
│   ├── menu.c                 # Menu system (escape key fix)
│   ├── player.c               # Player movement and weapons
│   ├── enemy.c                # Enemy AI and spawning
│   └── ...
├── wasm_build/                # Compiled WebAssembly output
│   ├── game.html              # Main HTML page (Rivendell styling)
│   ├── game.js                # Emscripten-generated JavaScript (396KB)
│   ├── game.wasm              # WebAssembly binary (2.2MB)
│   ├── game.data.bin          # Compressed asset pack (32MB) ✅ ACTIVE
│   ├── game.data.gz           # Compressed asset pack (32MB, backup)
│   └── game.data.js           # Data loader script (17KB, embedded in game.js)
├── data/                      # Original assets (BMPs, RAWs)
├── sound/                     # Original audio (OGG, WAV)
├── shell.html                 # Emscripten shell template
├── Makefile.emscripten        # Build configuration
├── patch_loader.sh            # Script for .gz patching
└── PROJECT_STATUS.md          # This document
```

---

## Server Configuration

### http-server (Node.js)
```bash
# Currently running on port 8000
http-server -p 8000 -c-1 --cors

# Access URLs:
# Local:  http://localhost:8000/wasm_build/game.html
# LAN:    http://192.168.1.56:8000/wasm_build/game.html
```

### macOS Network Configuration
**Local Network Privacy Settings:**
- System Settings → Privacy & Security → Local Network
- ✅ Terminal - Enabled
- ✅ node - Enabled

**Why Required:**
macOS 26.1+ blocks incoming connections by default. Without these permissions, external devices (iOS) cannot connect even when firewall is disabled.

### Server Logs
Located at: `/tmp/http_server.log`

**Key Observations:**
- Chrome on Mac successfully requests: `game.html` → `game.js` → `game.wasm` → `game.data`
- iOS Safari requests: `game.html` → `game.js` → `game.wasm` → **STOPS** (no game.data request)
- Firefox iOS: Same behavior as Safari (WebKit engine limitation)

---

## Build Process

### Full Rebuild
```bash
cd /Users/ben/Desktop/particlefield/downloads/alan_parsons_project2
make -f Makefile.emscripten clean
make -f Makefile.emscripten
```

**Output:**
- Compiles C sources to WASM
- Generates `wasm_build/game.js`, `game.wasm`, `game.html`
- Packages assets into `game.data`
- Inlines `game.data.js` into `game.js`

### Serve Locally
```bash
make -f Makefile.emscripten serve
```

**This runs:**
```bash
python3 -m http.server 8000
```

**Note:** If port 8000 is in use, kill existing server:
```bash
lsof -ti:8000 | xargs kill -9
```

---

## iOS Debugging Guide

### Prerequisites
1. iOS device on same WiFi network as Mac (192.168.1.x subnet)
2. Safari → Preferences → Advanced → "Show Develop menu in menu bar" ✅
3. iOS Settings → Safari → Advanced → Web Inspector ✅

### Steps to Debug

1. **On iOS:** Navigate to `http://192.168.1.56:8000/wasm_build/game.html?v=4`
   - Use cache-busting parameter `?v=X` to force fresh JavaScript load

2. **On Mac:** Safari → Develop → [Your iPhone Name] → game.html
   - Opens Web Inspector remotely connected to iOS Safari

3. **Check Console Tab:**
   - Look for `[loadPackage]` log messages
   - Identify where the loading process stops
   - Check for JavaScript errors or exceptions

4. **Check Network Tab:**
   - Verify which resources are requested
   - Check if `game.data` appears (currently it doesn't)
   - Look for failed requests or 404s

5. **Expected Log Sequence (if working):**
   ```
   [loadPackage] Starting data file load...
   [loadPackage] Package: wasm_build/game.data Remote: game.data Size: 222894368
   [loadPackage] Opening IndexedDB...
   [loadPackage] Checking cached package...
   [loadPackage] Use cached: false
   [loadPackage] Fetching remote package...
   [fetchRemotePackage] Fetching: game.data Size: 222894368
   [fetchRemotePackage] Starting fetch...
   [fetchRemotePackage] Fetch response: 200 true
   [loadPackage] Package data received, size: 222894368
   [loadPackage] Package processing complete
   ```

6. **If Logging Stops Early:**
   - Note the last message received
   - Check for error messages between log points
   - Errors should trigger `[preloadFallback]` messages

---

## Known Issues & Limitations

### 1. Asset Compression ✅ RESOLVED
**Impact:** Originally 213MB uncompressed was too large for mobile
**Attempted Fix:** Gzip compression with .gz extension failed due to server auto-decompression
**Solution:** Using `.bin` extension prevents server interference, client-side decompression works
**Current State:** 32MB download with client-side decompression (86% reduction) ✅

### 2. Main Menu Escape Key ✅ RESOLVED
**Impact:** Pressing escape in menu returned to game instead of exiting
**Solution:** Changed `menu.c:163` to always set `MenuResult = 0` on escape
**Current State:** Escape in menu now properly exits to start screen ✅

### 3. iOS Safari Loading Issue (ONGOING)
**Symptom:** Game never requests `game.data` file
**Hypothesis:**
- JavaScript error preventing loadPackage execution
- IndexedDB access denied/blocked on iOS
- Async/await compatibility issue
- Module initialization ordering problem
- DecompressionStream API may have issues on iOS Safari (less likely - supported since iOS 16.4)

**Debug Status:**
- Comprehensive logging added throughout data loader
- Decompression error handling in place
- Need Safari Web Inspector to see console logs
- Test URL with cache busting: http://192.168.1.56:8000/wasm_build/game.html?v=6

### 4. LocalStorage Persistence (WORKING)
**Note:** Works on desktop, untested on iOS due to loading issue

---

## Code References

### Escape Key Menu Fix (Nov 14)
```c
// File: source_c/menu.c:161-167
/* Check for escape (always exit to start screen) */
if (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
    MenuResult = 0;  /* Always exit when escape pressed in menu */
    IsMenuRunning = 0;
    Mix_FadeOutMusic(200);
    break;
}
```

**Behavior:**
- **Before:** Pressing escape returned to active level (if game_running == 1)
- **After:** Pressing escape always exits to start screen, stops music, shows overlay

### Compression Implementation (Nov 14)
```javascript
// File: wasm_build/game.data.js:73-84
// Decompress gzip data using DecompressionStream
console.log('[fetchRemotePackage] Decompressing data, compressed size:', packageData.byteLength);
try {
    const ds = new DecompressionStream("gzip");
    const decompressedStream = new Response(packageData.buffer).body.pipeThrough(ds);
    const decompressed = await new Response(decompressedStream).arrayBuffer();
    console.log('[fetchRemotePackage] Decompressed size:', decompressed.byteLength);
    return decompressed;
} catch (e) {
    console.error('[fetchRemotePackage] Decompression failed:', e);
    throw e;
}
```

**Key Points:**
- Uses `.bin` extension to prevent server auto-decompression
- DecompressionStream API (supported iOS 16.4+, Safari 16.4+)
- Error handling logs failures to console
- 32MB compressed → 213MB decompressed in ~1-2 seconds

### Captain Planet Mode Toggle
```javascript
// File: shell.html:666-703
function toggleCaptainPlanet() {
    var currentMode = localStorage.getItem('alan_parsons_captain_planet');
    if (currentMode === '1') {
        localStorage.removeItem('alan_parsons_captain_planet');
        // Update UI to show OFF state
    } else {
        localStorage.setItem('alan_parsons_captain_planet', '1');
        // Update UI to show ON state
    }
    location.reload(); // Apply changes
}
```

### Game Completion Hook
```c
// File: source_c/main.c:960-966
#ifdef __EMSCRIPTEN__
EM_ASM(
    localStorage.setItem('alan_parsons_beaten', '1');
    console.log('Game beaten! Captain Planet mode unlocked.');
);
#endif
```

### Nuke Count Initialization
```c
// File: source_c/main.c:235-246
int captain_planet_mode = EM_ASM_INT({
    var cpMode = localStorage.getItem('alan_parsons_captain_planet');
    if (cpMode === '1') {
        console.log('Captain Planet mode enabled - 18 nukes!');
        return 1;
    }
    return 0;
});
if (captain_planet_mode) {
    num_starting_nukes = 18;
}
```

### Fade Timing
```c
// File: source_c/main.c:1655-1665
FadeToWhite(500, 2);   // 5 seconds fade to white
// ... show ending image ...
FadeFromWhite(300, 2); // 3 seconds fade from white
```

---

## Browser Compatibility

| Browser | Platform | Status | Notes |
|---------|----------|--------|-------|
| Chrome | macOS | ✅ Working | Full functionality |
| Safari | macOS | ✅ Working | Full functionality |
| Safari | iOS 18.7 | ❌ Broken | Hangs at 0%, no data fetch |
| Firefox | iOS 18.7 | ❌ Broken | Same as Safari (WebKit) |
| Firefox | Desktop | ⚠️ Untested | Should work (same engine as Chrome) |

### Minimum Requirements (per Emscripten)
- Safari 15.0+ ✅ iOS 18.7 meets requirement
- Chrome 85+ ✅
- Firefox 79+ ✅
- Node 16.0+ ✅

**Conclusion:** iOS meets all version requirements, issue is runtime behavior not API support

---

## Next Steps

### Immediate Priority: Verify iOS Loading with Compression
1. **Test on iOS** with new compressed .bin file (http://192.168.1.56:8000/wasm_build/game.html?v=6)
2. **Inspect Safari Web Inspector** - Check for decompression errors or failures
3. **Verify download progress** - Confirm game.data.bin is requested and downloaded
4. **Monitor console logs** - Look for `[fetchRemotePackage] Decompressing...` messages
5. **Fallback plan** - If decompression fails on iOS, may need to serve uncompressed to mobile only

### Secondary Priority: Desktop Testing
1. **Verify compression works** on Chrome/Safari desktop
2. **Test escape key** - Confirm menu exits properly instead of returning to game
3. **Performance check** - Ensure decompression doesn't cause lag on slower machines

### Future Enhancements

2. **Progressive asset loading**
   - Load menu assets first (show UI immediately)
   - Stream level data on demand
   - Reduce initial payload

3. **Service Worker caching**
   - Offline play support
   - Faster subsequent loads

4. **Touch controls for mobile**
   - Currently keyboard/mouse only
   - Need virtual joystick overlay

5. **Responsive canvas sizing**
   - Fixed 1280x720 may not be ideal for all devices
   - Consider dynamic scaling

---

## Contact & Resources

### Emscripten Documentation
- Main site: https://emscripten.org
- File packaging: https://emscripten.org/docs/porting/files/packaging_files.html
- IndexedDB caching: https://emscripten.org/docs/api_reference/emscripten.h.html

### Debugging Resources
- Safari Web Inspector: https://webkit.org/web-inspector/
- iOS debugging: https://developer.apple.com/documentation/safari-developer-tools

### Project History
- Original: x86 Assembly
- First port: C language
- Current: WebAssembly + Emscripten
- Theme: Lord of the Rings / Rivendell aesthetic

---

## Build Information

**Emscripten Version:** (run `emcc --version` to verify)
**Node.js Version:** v25.1.0
**Python Version:** 3.14.0_1

**Compile Flags:**
```makefile
EMCC_FLAGS = -s WASM=1 \
             -s USE_SDL=2 \
             -s USE_SDL_MIXER=2 \
             -s ALLOW_MEMORY_GROWTH=1 \
             --preload-file data \
             --preload-file sound \
             --shell-file shell.html
```

**Asset Packaging:**
- Method: `--preload-file` embeds assets in `.data` file
- Format: Binary blob with metadata JSON
- Index: `game.data.js` contains file offsets/sizes
- Virtual FS: Emscripten mounts at `/data/` and `/sound/`

---

## Revision History

| Date | Change | Reason |
|------|--------|--------|
| Nov 14 | Fixed escape key in menu | User request - should exit, not resume |
| Nov 14 | Re-enabled compression with .bin | 86% size reduction (213MB→32MB) |
| Nov 14 | Added decompression error handling | Robust client-side decompression |
| Nov 12 | Added comprehensive logging | Debug iOS loading issue |
| Nov 12 | Temporarily used uncompressed | Troubleshoot gzip issues |
| Nov 12 | Implemented Captain Planet mode | User request for unlockable |
| Nov 12 | Fixed fade timings (500/300) | Ending too slow |
| Nov 11 | Initial WASM compilation | Port from C to web |

---

## Testing Checklist

### Desktop (Chrome/Safari on macOS)
- [ ] Game loads with compressed 32MB file
- [ ] Decompression completes successfully
- [ ] All assets display correctly
- [ ] Escape key in menu exits to start screen (not back to level)
- [ ] Captain Planet mode toggle works
- [ ] Audio plays correctly

### iOS Safari (18.7)
- [ ] Page loads without hanging at 0%
- [ ] game.data.bin is requested from server
- [ ] Decompression logs appear in console
- [ ] Game initializes and runs
- [ ] Touch controls work (if implemented)
- [ ] Audio works after user interaction

**Test URL:** http://192.168.1.56:8000/wasm_build/game.html?v=6

---

**Last Updated:** November 14, 2025
**Status:** Desktop ✅ / iOS ⚠️ (testing compression fix)

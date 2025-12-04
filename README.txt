
**********The Alan Parsons Project**********

----Modern Web Build (WASM)----

This repo now includes a direct NASM → WebAssembly toolchain.  To rebuild the
browser version:

1. `./build.sh` – converts every `source/*.asm` module into a single
   `build/game.wasm` (no merged ASM file required anymore).
2. `./clean.sh` – removes generated `.wat/.wasm` and symbol maps so the next
   build starts fresh.
3. `node build/test_wasm.js` – runs a smoke test against the produced
   `game.wasm` using the same runtime stubs that power `build/index.html`.

The JavaScript runtime (`build/game.js`) now emulates the SDL/SDL_mixer APIs
used by the original 2003 code: `_LoadBMP` pulls from cached browser assets,
keyboard/mouse buffers are shared directly with the WASM memory, and audio is
played via the Web Audio API with per-channel start/stop/fade support.  When
running in a browser, open `build/index.html` (via any static file server) and
the loader will fetch `build/game.wasm`, preload assets from `build/data/` and
`build/sound/`, and hand them to the WASM game exactly where the SDL build
expects them.


----Pre-Requisites:

NOTE: I've included the required libraries in the directory labeled SDL.
They may not all be needed but I don't know since I basically
installed all of them before I even began porting the project.
**********
-To run the game in either windows or linux you must have the SDL
 runtime libraries installed. (www.libsdl.org)

-To compile the game in either windows or linux you must have the SDL
 development libraries installed. (www.libsdl.org)
**********


----Running (In Linux):
-Open a terminal and navigate to the alan parsons project directory
-If you need to make the executable type "make"
-To run the game in windowed mode type "./linux_app"
-To run the game in fullscreen mode type "./linux_app --fullscreen"


----**Running (In Windows):
-Open a command prompt and navigate to the alan parsons project directory
-*If you need to make the executable type "make"
-To run the game in windowed mode type "./win_app"
-To run the game in fullscreen mode type "./win_app --fullscreen"

*NOTE: To compile in windows you must have NASM and gcc 3.1 or later
       installed.

** I haven't figured out how to compile the stupid thing in windows yet.
   As far as I can tell...you'll need gcc and nasm installed, and you'll need
   to modify the Makefile to do linking correctly.
   Installing the SDL libraries is not a problem, but linking with them and
   the libC libraries *is*.

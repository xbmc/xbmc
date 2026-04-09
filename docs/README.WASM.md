# Build Kodi for WASM (Emscripten) - WIP

This document tracks the current WebAssembly platform bring-up status.

The WASM target is not fully working yet. This guide documents what is already integrated and what remains to be done.

## Current Status

- `tools/depends` now has a WASM host path for `wasm32-unknown-emscripten`.
- Generated toolchain files are produced under the WASM depends prefix (`Toolchain.cmake`, `config.site`, Meson cross file).

## Build

```bash
source "$EMSDK/emsdk_env.sh"

cd "$HOME/kodi/tools/depends"
sh bootstrap
autoreconf -fi
./configure \
  --host=wasm32-unknown-emscripten \
  --with-platform=wasm \
  --prefix=/tmp/kodi-wasm-depends \
  --disable-debug
make -j$(getconf _NPROCESSORS_ONLN)
```


# Tizen web packaging template for Kodi WASM

Source-controlled template files used to stage a Tizen web app from a WASM Kodi
build.

The build, packaging and install workflow is documented in
[`docs/README.WASM.md`](../../../docs/README.WASM.md); SDK, device pairing and
certificate setup in [`docs/README.Tizen.md`](../../../docs/README.Tizen.md).
This file only describes what is in this directory and how it is staged.

## Files

- `icon.png`: application icon shown in the TV's Apps panel; 512x423 24-bit RGB PNG as Samsung requires, rendered from `tools/Linux/packaging/media/iconScalable.svg`.
- `config.xml`: Tizen widget metadata. Holds the application and package IDs
  (`kodiplayer.Kodi` / `kodiplayer`), the `tv-samsung` profile, the widget
  version and the privileges Kodi needs: `internet`, and the public-level
  Samsung `network.public` and `productinfo` privileges that let the System
  Information window show the TV's network configuration, model and firmware.
- `tizen_web_project.yaml.in`: CMake-configured Tizen web project config. Its
  `signing_profile` is intentionally empty, which makes `tz` sign with whichever
  security profile is active.
- `.project` / `.tproject`: Tizen Studio project descriptors, staged so the
  output directory can also be opened directly in the IDE.

The staged `index.html` is copied from the shared browser shell at
`tools/wasm/kodi.html` so browser and Tizen builds use the same entry page.

## Staging

`cmake --build <build-dir> --target package_tizen` assembles a complete Tizen
web project in `<build-dir>/tizen_packaging/Kodi`:

- the template files from this directory
- `tizen_web_project.yaml`, generated from the CMake template
- `index.html`, the shared web entry page
- `media/splash.jpg`, the splash picture `index.html` shows while Kodi loads
- the build artifacts `kodi.js`, `kodi.wasm` and `kodi.data`

`package_tizen_wgt` then packs and signs that directory into
`<build-dir>/tizen_packaging/Kodi/Debug/Kodi.wgt`, and `install_tizen_wgt`
pushes it to a connected device. Both require the `tz` CLI to have been found
when CMake configured the build; the values that name the staging directory,
output name, profile and API version live in
`cmake/scripts/wasm/TizenTargets.cmake`.

## Profiling

Configure the build with `-DENABLE_WASM_PROFILING=ON` to preserve wasm function
names, then build and install with
`cmake --build <build-dir> --target install_tizen_wgt`.
Close Kodi before running `tools/wasm/tizen/inspect.sh`, then attach DevTools
as described in [`docs/README.WASM.md`](../../../docs/README.WASM.md#74-launch-debug-and-remove).

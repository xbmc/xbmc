# Kodi E2E tests

End-to-end (E2E) test harness for Kodi, described in
**[docs/E2E-TESTING.md](../../docs/E2E-TESTING.md)**.

> [!NOTE]
> This is the first slice, not a complete test suite. It currently covers the core
> mechanics: build Kodi from source, launch it with a disposable profile, confirm the
> JSON-RPC webserver responds, take and sanity-check a screenshot, and shut it down
> cleanly. See `docs/E2E-TESTING.md` for the full phased plan.

## What this does

Every scenario launches a built `kodi.bin`/`kodi` binary, or (if `KODI_APK` is set) an
Android APK on an adb-connected device/emulator, or (if `KODI_APP` is set) an iOS
`.app` on a Simulator, via the shared `kodi` fixture in `conftest.py`. Either way it
gets a throwaway profile pre-seeded with the webserver enabled on a non-default port,
authentication disabled, and a screenshot output directory configured - a disposable
`portable_data` profile (`-p`) on desktop, a wiped `$HOME/.kodi/userdata` on Android
(see `driver/android_launcher.py`) or a wiped Simulator app-container `userdata` on
iOS (see `driver/ios_launcher.py`), neither of which has a `-p` flag to rely on.
Every scenario ends by asking Kodi to quit via `driver/assertions.py`'s
`assert_clean_shutdown`, which asserts a clean (`0`) exit code and scans the log for
fatal-level lines - so a crash or hang fails loudly regardless of which scenario was
running. Those lines read as either `FATAL` or `critical` depending on how spdlog was
built, and both are matched; see the comment in `driver/assertions.py`.

`scenarios/test_startup.py`:

1. Waits for the webserver port to open.
2. Calls `JSONRPC.Ping` over HTTP JSON-RPC and asserts the `"pong"` response.

`scenarios/test_screenshot.py`:

1. Pings, then waits for `GUI.GetProperties(currentwindow)` to report the Home window -
   a coarse fast-fail gate, not a "finished rendering" signal (Kodi flips
   `currentwindow` the instant window activation *starts*, before that window's XML
   layout/controls/textures actually load - see the module docstring).
2. Triggers a screenshot via `Input.ExecuteAction` (`{"action": "screenshot"}`, the
   same action a "screenshot" keybinding would send), waits for the resulting PNG to
   appear and finish being written, and checks it isn't a single solid color. Since
   there's no reliable "done rendering" signal to wait for, this retries on a generous
   budget rather than asserting on the first attempt - the software-rendering
   fallback's first paint can be slow.
3. Asserts the final screenshot is a valid, plausibly-sized image that isn't a single
   solid color - catching e.g. a GL context that creates successfully but renders
   nothing. This is a basic sanity check, not pixel/visual regression testing (Phase 2 in
   `docs/E2E-TESTING.md`).

`scenarios/test_navigation.py`:

1. Pings, waits for the Home window, then calls `GUI.ActivateWindow(window="settings")`
   and waits for `currentwindow` to report the settings menu.
2. Calls `Input.Back` and waits for `currentwindow` to report Home again.

Deliberately uses `GUI.ActivateWindow`/`Input.Back` rather than scripted
`Input.Down`/`Input.Select` keypresses: which skin element is focused first is
skin/version-specific, so blindly navigating by direction would make the test fragile
in a way unrelated to what it's meant to catch. See the module docstring for the
reasoning.

`scenarios/test_settings.py`:

1. Pings, reads `lookandfeel.enablerssfeeds` via `Settings.GetSettingValue` and asserts
   it's the documented default (`false`).
2. Sets it to `true` via `Settings.SetSettingValue`, reads it back, and asserts the
   change persisted.

## Layout

- `conftest.py` — the shared `kodi` fixture (launches/tears down a `KodiProcess`,
  `AndroidKodiProcess` if `KODI_APK` is set, or `IOSKodiProcess` if `KODI_APP` is
  set, per test) and `KODI_BINARY` resolution, used by every scenario.
- `driver/launcher.py` — starts/stops a Kodi process with a disposable portable
  profile. Exercised on macOS, Linux (POSIX), and Windows in CI; not Kodi/OS-specific
  beyond that (kodi.exe supports `-p`/`--portable` the same way, so Windows needed no
  launcher changes at all).
- `driver/android_launcher.py` — the Android equivalent: installs a built debug APK
  over adb, launches it on a device/emulator, and manages the same disposable-profile
  contract (fresh userdata per test, webserver enabled, screenshot path set) despite
  Android Kodi having no `-p`/`--portable` flag to ask for that directly.
- `driver/ios_launcher.py` — the iOS/tvOS equivalent: installs a built `.app` via
  `xcrun simctl` on a Simulator device and manages the same disposable-profile
  contract. Also has no `-p`/`--portable` flag, but unlike Android the Simulator
  shares the host filesystem/network directly, so there's no adb-style push/pull/
  forward - the app's data container (`simctl get_app_container`) is just a local
  path. Generic over both platforms (bundle id read from `Info.plist`, no iOS- or
  tvOS-specific code), so the tvOS job reuses it unmodified.
- `driver/kodi_client.py` — a minimal HTTP JSON-RPC client (no external Kodi-specific
  dependency). As the suite grows past simple request/response calls (e.g. waiting on
  `Player.OnPlay` notifications), see `docs/E2E-TESTING.md` for the recommendation to
  adopt `jsonrpc-websocket`/`pykodi` instead of extending this by hand.
- `driver/assertions.py` — shared post-scenario assertions (currently just
  `assert_clean_shutdown`), used by every scenario.
- `scenarios/` — the actual pytest test cases, shared unmodified across every
  platform/launcher.

## Running locally

Build Kodi first (see `docs/README.macOS.md` / `docs/README.Linux.md` /
`docs/README.Windows.md` - `KODI_BINARY` works the same way pointing at `kodi.exe`, no
Windows-specific env var needed), then, using [`uv`](https://docs.astral.sh/uv/)
(dependencies are declared in `pyproject.toml` / `uv.lock`, no manual virtualenv setup
needed):

```bash
cd tools/e2e
KODI_BINARY=/path/to/kodi.bin uv run pytest scenarios -v
```

If `KODI_BINARY` is not set, it defaults to `<repo_root>/build/kodi.bin`. To run
against Android instead, set `KODI_APK` to a built debug APK (`make apk` per
`docs/README.Android.md`) with a device or emulator already connected over adb
(`ANDROID_SERIAL` selects a specific one if more than one is attached):

```bash
cd tools/e2e
KODI_APK=/path/to/kodiapp-x86_64-debug.apk uv run pytest scenarios -v
```

To run against an iOS or tvOS Simulator, build with `--with-platform=ios-simulator`
or `--with-platform=tvos-simulator` (see `docs/README.iOS.md`/`docs/README.tvOS.md`),
boot a Simulator device, and set `KODI_APP` to the built `.app`
(`KODI_SIMULATOR_DEVICE` selects a specific device UDID; defaults to `booted`):

```bash
cd tools/e2e
KODI_APP=/path/to/Kodi.app uv run pytest scenarios -v
```

## CI

Six workflows run this suite, all triggered manually
(`workflow_dispatch`) or by pushes touching `tools/e2e/**` rather than wired into
every PR, since build time and reliability haven't been proven out yet:

- `.github/workflows/e2e-macos.yml` — builds Kodi from source (via
  `tools/depends`, cached) on a GitHub-hosted macOS runner and runs the suite
  against the `Kodi.app` bundle.
- `.github/workflows/e2e-linux.yml` — builds Kodi's GBM windowing backend
  on a GitHub-hosted Ubuntu runner, using `tools/depends` (cached, like the macOS
  job) for the GBM/media dependency stack (mesa, libdrm, libinput,
  libdisplay-info, ffmpeg, dav1d), plus a handful of system packages for
  libraries `tools/depends` doesn't build on Linux. Runs on a virtual KMS display
  (the `vkms` kernel module) with Mesa built for software GLES rendering,
  exercising the real DRM/GBM/EGL/GLES pipeline on a headless GPU-less runner.
- `.github/workflows/e2e-android.yml` — cross-builds the Android debug APK from
  source (via `tools/depends --host=x86_64-linux-android`, cached), boots a
  hardware-accelerated x86_64 emulator (`reactivecircus/android-emulator-runner`),
  installs the APK, and runs the suite over an adb-forwarded JSON-RPC connection.
- `.github/workflows/e2e-apple-simulator.yml` — cross-builds the iOS app for the Simulator
  ABI from source (via `tools/depends --host=aarch64-apple-darwin
  --with-platform=ios-simulator`, cached, then `xcodebuild`), creates and boots a
  Simulator device via `xcrun simctl`, installs the app, and runs the suite over a
  direct (unforwarded) JSON-RPC connection. The Simulator ABI is far less exercised
  than the device one, so treat build failures here as plausibly a build-system gap,
  not just a test/CI-wiring issue.
- `.github/workflows/e2e-apple-simulator.yml` — same approach as the iOS job, reusing
  `driver/ios_launcher.py` unmodified since it's generic over any darwin_embedded
  Simulator app, built via `--with-platform=tvos-simulator`.
- `.github/workflows/e2e-windows.yml` — builds Kodi from source on a GitHub-hosted
  Windows runner and runs the suite against `kodi.exe` directly. Unlike the other
  source-built jobs, dependencies aren't compiled via `tools/depends` at all - they're
  downloaded as prebuilt packages from `mirrors.kodi.tv`
  (`tools/buildsteps/windows/download-dependencies.bat`) plus an MSYS2 environment for
  the rest (`download-msys2.bat`), then built via `cmake --build` (the Visual Studio
  generator), per `docs/README.Windows.md`.

### Android: reaching the on-device profile

Android Kodi has no `-p`/`--portable` equivalent, so `driver/android_launcher.py`
seeds a fixed on-device profile instead of handing Kodi a disposable one. On API 30
that profile lives on *external* storage - `$HOME` is `getExternalFilesDir("")`, i.e.
`/storage/emulated/0/Android/data/org.xbmc.kodi/files/.kodi` - and getting at it from
adb is genuinely awkward. All three obvious approaches fail, each in its own way, and
each was confirmed against CI rather than assumed:

| Approach | What happens |
| --- | --- |
| `adb shell run-as <pkg>` | Runs as the app's UID but inside adbd's mount namespace, which has no per-app view of `/storage`. Everything fails with `mkdir: '/storage/emulated': Permission denied`. It only ever reaches *internal* storage. |
| `adb shell` (uid `shell`) | Refused outright by scoped storage - even reading a file Kodi is actively writing gives `failed to stat remote object ... Permission denied`. |
| `adb root` | Gets in, but writes bypass the storage FUSE daemon and land in the lower filesystem, so new files are labelled `storage_file` rather than `media_rw_data_file`. SELinux then denies the app access to its *own* profile (`avc: denied { getattr } ... scontext=u:r:untrusted_app ... tcontext=u:object_r:storage_file`) and Kodi aborts on `unable to load settings`. |

What the launcher does instead: **root for access, but never create anything out
there.** A priming launch lets Kodi build its own profile tree (with no
`guisettings.xml` it starts on defaults with the webserver off, which is far enough),
and seeding then truncates the file Kodi just wrote (`cat staged > existing`) rather
than pushing a new one over it - same inode, so the owner and SELinux label survive.
Screenshots go to `special://temp/` for the same reason: it is a directory Kodi
creates, so the launcher does not have to. Test isolation still wipes `userdata`;
deleting as root is fine, and the priming launch is what brings the files back
correctly labelled.

Note that `adb root` only works on userdebug emulator images (`google_apis`, not
`google_apis_playstore`) and on no retail device, so this approach is specific to the
emulator this job boots.

## Known limitations / not yet covered

- No test media / playback testing yet.
- The Linux GBM job renders through Mesa's software rasterizer on a virtual KMS
  device, so it validates Kodi's windowing/EGL/GLES code paths but not real GPU
  drivers or HW video decode (V4L2/VAAPI).
- The Android job builds x86_64 (for emulator hardware acceleration), not arm64 -
  the ABI real Android devices actually ship - and runs on a phone/tablet emulator
  profile, not an Android TV one, so it doesn't cover Kodi's leanback/TV UI paths.
- The iOS and tvOS jobs build for the Simulator, not real device hardware, so like
  Android's emulator they don't cover real GPU drivers or hardware video decode
  (VideoToolbox) - and unlike the other jobs' build configurations, the Simulator
  configurations themselves are new and unproven.
- The screenshot check is a "did anything render at all" sanity check, not
  pixel/visual regression testing against a baseline (Phase 2).
- Binary add-ons are not built (`tools/depends/target/binary-addons` step is skipped),
  so this only proves core startup, not add-on-dependent functionality.

See [TEST_BACKLOG.md](TEST_BACKLOG.md) for a longer list of candidate tests distilled
from recent `xbmc/xbmc` issue triage - split between unit/component test candidates,
E2E tests addable with the current harness, and E2E tests that need new fixtures/infra
first - and [COVERAGE_MATRIX.md](COVERAGE_MATRIX.md) for broader feature-area E2E
coverage organized by Kodi subsystem rather than by individual bug report.

# Test backlog from recent issue triage

The lists below came from scanning the ~100 most recently opened issues on
[github.com/xbmc/xbmc/issues](https://github.com/xbmc/xbmc/issues) (as of 2026-07-22)
for classes of bugs this E2E harness's architecture (JSON-RPC control, screenshot
sanity-check, clean-shutdown assertion, per-platform launchers - see
[README.md](README.md)) could plausibly have caught.

Not everything below belongs in `tools/e2e` - a chunk of these are pure logic/parsing
bugs that a `xbmc/test` gtest can reproduce far faster and more reliably than launching
a full GUI-rendering app, so they're called out separately first. The rest are split by
how much new infrastructure they'd need beyond what `driver/` and `scenarios/` already
provide.

## Unit/component test candidates (`xbmc/test/`, not this E2E suite)

These bugs live in code that doesn't require a rendered GUI to exercise - a
component-level gtest can hit the same code path directly and run in milliseconds
instead of a full app launch/teardown cycle.

- **Settings dependency evaluation** — `CSettingDependency`/`CSettingsManager` logic,
  no window needed. [#28283](https://github.com/xbmc/xbmc/issues/28283) (`list[string]`
  multiselect shorthand dependency logs an "unknown setting" warning).
- **Webserver auth middleware** — `CWebServer`'s auth check can be exercised with a
  mocked HTTP request/response, no GUI process required. Worth a direct unit test
  regardless of any issue, since it's a security-relevant code path this E2E suite's
  fixture profile always seeds with auth *disabled*.
- **Video library scan/clean DB logic** — `CVideoDatabase`/`CVideoInfoScanner`
  scanning and cleanup is DB logic, not rendering.
  [#28587](https://github.com/xbmc/xbmc/issues/28587) (Blu-ray/DVD folders not
  removed from the library when their source is removed),
  [#28291](https://github.com/xbmc/xbmc/issues/28291) (cleaning the library hangs on
  a removed NFS share), [#28566](https://github.com/xbmc/xbmc/issues/28566) (can't
  add an empty TV series).
- **Addon version string parsing** — pure parsing, easiest possible unit test.
  [#28057](https://github.com/xbmc/xbmc/issues/28057) (`SIGSEGV` in
  `CAddonVersion::CAddonVersion` parsing a repository addon version string).
- **Plot/metadata text handling** — string-handling/DB logic.
  [#28069](https://github.com/xbmc/xbmc/issues/28069) (movie plot gets line breaks
  when combining Chinese and English text).
  [#27899](https://github.com/xbmc/xbmc/issues/27899) ("Unable to obtain time zone
  bias!" triggered by `tvshow.premiered = 1969-12-31`) is the same flavor of
  edge-case-input bug.
- **Tag parsing** — `taglib` wrapper logic, mockable with fixture tag files, no app
  launch needed. [#28606](https://github.com/xbmc/xbmc/issues/28606) (MKA files album
  name issue), [#28182](https://github.com/xbmc/xbmc/issues/28182) (multiple genres
  not shown for M4A/DSF files).
- **`CCurlFile::Exists()`/`Stat()` retry behavior** — mock the curl transport and
  assert a transient error doesn't get misreported as "file doesn't exist".
  [#28382](https://github.com/xbmc/xbmc/issues/28382) (a transient curl error on the
  pre-play probe aborts WebDAV/`davs://` playback).
- **VFS-layer correctness bugs** — the *parsing/protocol* half of the network
  filesystem issues below is a `CNFSFile`/`CSMBFile` component test against a mock
  server, not a GUI concern.
  [#28265](https://github.com/xbmc/xbmc/issues/28265) (NFSv4 `nfs_readlink` returns
  garbage characters at the end of the buffer). (The *GUI responsiveness* half of the
  SMB/NFS complaints stays in the E2E bucket below.)

## Quick E2E wins (current harness, no new fixtures)

These genuinely need a running, rendered app - the bug is about window/lifecycle
behavior, not an isolated function - but are achievable with what `driver/` and
`scenarios/` already provide today.

- **File manager navigation smoke test** — `activate_window("filemanager")` +
  `wait_for_window` + `Input.Back`, mirroring what `test_navigation.py` already does
  for Settings. Cheap general coverage for the "filelist freezes/misbehaves" class,
  e.g. [#28525](https://github.com/xbmc/xbmc/issues/28525) (slow return from an SMB
  share's filelist - the responsiveness half of this bug, as opposed to the
  `nfs_readlink` parsing half above) and
  [#28421](https://github.com/xbmc/xbmc/issues/28421) (can't create multipath
  directories via the file manager).
- **Add-on browser open/close smoke test** — same pattern
  (`activate_window("addonbrowser")` + back), catching general "opening this window
  hangs the GUI" regressions like
  [#28032](https://github.com/xbmc/xbmc/issues/28032) (reproducible UI freeze on
  "install missing addon") without needing a real add-on repo fixture.
- **Multi-window navigate-then-quit fuzz** — loop `activate_window` across
  `home`/`settings`/`filemanager`/`addonbrowser` before calling `assert_clean_shutdown`,
  to catch "crashes on exit only after visiting window X" regressions such as
  [#27758](https://github.com/xbmc/xbmc/issues/27758) (crash on exit in
  `CGUIControlLookup::RemoveLookup`), [#28368](https://github.com/xbmc/xbmc/issues/28368)
  (crashing when trying to close), and
  [#28601](https://github.com/xbmc/xbmc/issues/28601) (process doesn't actually exit,
  "needs to be killed"). This is app-lifecycle/teardown-ordering behavior, not
  something a unit test can reproduce - it depends on the real window stack built from
  skin XML.

## Needs more work (new fixtures/infra first, still E2E)

- **Playback smoke suite** (`test_playback.py`, the still-unimplemented Phase 1 item
  from `docs/E2E-TESTING.md`) — needs the small h264/hevc/audio/subtitle fixture set
  the doc already calls for: `Player.Open`, poll `Player.GetProperties` for
  `speed`/`time` advancing, `Player.Stop`. This is the single highest-value gap: a
  large fraction of current open issues are playback-adjacent and today's suite has
  zero playback coverage, e.g.
  [#28510](https://github.com/xbmc/xbmc/issues/28510) (slower playback start/end in
  v22 beta1 than alpha3), [#28117](https://github.com/xbmc/xbmc/issues/28117) (forced
  subtitles fail to auto-display), [#28603](https://github.com/xbmc/xbmc/issues/28603)
  (PGS subtitle colors wrong on HDR10/Dolby Vision on Android),
  [#27743](https://github.com/xbmc/xbmc/issues/27743)
  (`VideoPlayer.IsStereoscopic` not set when starting 3D playback from the info
  dialog), and [#28638](https://github.com/xbmc/xbmc/issues/28638) (HDR/SDR sync delay
  configurability).
- **Optical disc (DVD/Blu-ray) menu smoke test** — needs an ISO/BDMV fixture and
  platform-specific mounting/playback, plus `Input.*` menu navigation. Relates to
  [#28654](https://github.com/xbmc/xbmc/issues/28654) (macOS crash showing the
  Blu-ray menu), [#28559](https://github.com/xbmc/xbmc/issues/28559) (Android DVD
  menu renders as a black screen), [#27943](https://github.com/xbmc/xbmc/issues/27943)
  (inserting a DVD/BD opens a file dialog instead of playing), and
  [#28392](https://github.com/xbmc/xbmc/issues/28392) (freeze entering the Disc home
  screen).
- **Mock PVR backend** — a small stub backend (or a scriptable PVR client add-on) so
  `PVR.*`/EPG JSON-RPC methods have something real to talk to. Relates to
  [#28614](https://github.com/xbmc/xbmc/issues/28614) (closed captions over the
  NextPVR backend), [#28561](https://github.com/xbmc/xbmc/issues/28561) (AC-3
  passthrough not engaged for live TVHeadend HTSP playback, but works from a
  recording of the same stream), and
  [#28110](https://github.com/xbmc/xbmc/issues/28110) (crash-to-desktop watching live
  TV over HDHomeRun).
- **Local add-on repository fixture** for install/uninstall/upgrade flows — a tiny
  locally-served repo with one or two purpose-built minimal add-ons. Relates to
  [#28032](https://github.com/xbmc/xbmc/issues/28032) (UI freeze installing a missing
  add-on), [#28478](https://github.com/xbmc/xbmc/issues/28478) (old add-on files not
  deleted after a Windows upgrade), and
  [#28581](https://github.com/xbmc/xbmc/issues/28581) (`xbmcgui`'s
  `setLabel(font=...)` never changes the rendered font - would need a minimal script
  add-on harness to exercise, since it's only visible in actual rendered output).
- **Visual/perceptual regression baselines** (the Phase 2 item `docs/E2E-TESTING.md`
  already calls out, e.g. `scikit-image`/`pixelmatch` + `pytest-mpl`/`syrupy`) —
  concrete evidence it'd catch real bugs:
  [#28639](https://github.com/xbmc/xbmc/issues/28639) (Estuary font rendering
  regressed on Windows as of a specific commit) and
  [#28059](https://github.com/xbmc/xbmc/issues/28059) (weather multiimage control
  visual glitch, Piers-only).
- **Android background/foreground lifecycle test** — needs a new
  `android_launcher.py` capability to send the app to background
  (`input keyevent KEYCODE_HOME` or an `am start` Home intent) and resume it, plus a
  longer idle wait. Relates to
  [#28560](https://github.com/xbmc/xbmc/issues/28560) (Android v22 beta1 crashes
  ~90s in), [#28372](https://github.com/xbmc/xbmc/issues/28372) (dialogs submitted
  with an empty value when app focus changes), and
  [#28238](https://github.com/xbmc/xbmc/issues/28238) (double instances on Android).
- **Network filesystem source test** (SMB/NFS) against a local mock server - the
  responsiveness/hang half of the SMB/NFS complaints (as opposed to the pure
  protocol-parsing half moved to the unit-test bucket above). Already flagged as
  future work in `README.md` ("No PVR/live-TV or UPnP/SMB network source testing
  yet"), worth calling out concretely given
  [#28502](https://github.com/xbmc/xbmc/issues/28502) (NFS/scraper issue on Nvidia
  Shield) alongside [#28525](https://github.com/xbmc/xbmc/issues/28525) above.
- **Android content-provider security boundary test** — the current driver only
  speaks JSON-RPC, but
  [#28479](https://github.com/xbmc/xbmc/issues/28479) (pre-auth, zero-permission
  arbitrary file read via `XBMCFileContentProvider`) is a real filed security bug
  reachable only through Android's `ContentProvider` IPC path, not JSON-RPC - would
  need a driver extension that talks to the provider directly (e.g. via `adb shell
  content query`) to ever catch a regression like it.

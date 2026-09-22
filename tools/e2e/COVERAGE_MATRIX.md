# E2E coverage matrix

Where [TEST_BACKLOG.md](TEST_BACKLOG.md) lists tests aimed at specific bugs pulled from
recent issue triage, this file takes the opposite angle: broad feature-area coverage
for the E2E suite, organized by Kodi subsystem rather than by individual bug report,
aimed at maximizing how much of the app a CI run exercises rather than reproducing one
regression. Everything here needs a real running, rendered instance - none of it is a
unit-test candidate.

Roughly cheapest-to-build-first: the JSON-RPC namespace sweep and full window sweep
need nothing beyond what `driver/`/`scenarios/` already provide; settings-category and
library-CRUD-for-music/pictures need only parametrizing existing scenarios; the
playback matrix, add-on lifecycle, and skin switching depend on fixtures not yet
built (see `TEST_BACKLOG.md`'s "needs more work" section, e.g. the playback fixture
set and the local add-on repo).

## Application lifecycle

- **Full window sweep** — `activate_window` through every top-level window
  (`videos`, `music`, `pictures`, `weather`, `addons`, `favourites`, `profiles`, …),
  not just Settings. One cheap loop that turns "does this window even open" into
  blanket coverage instead of the two hand-picked windows tested today.
- **Restart round trip** (`Application.Restart`/equivalent builtin) — a distinct code
  path from cold start + quit, currently untested.
- **Userdata-portability / schema-migration test** — launch against a `userdata` dir
  seeded from an older DB/settings schema (checked into the fixture set) and confirm
  migration completes and the app stays usable. A broad net for "this release's DB
  migration breaks upgraders," which today's always-fresh profile can never catch.

## JSON-RPC surface breadth

- **One read-only smoke call per top-level namespace** (`System.*`, `Player.*`,
  `Playlist.*`, `Files.*`, `VideoLibrary.*`, `AudioLibrary.*`, `PVR.*`, `Addons.*`,
  `GUI.*`, `Profiles.*`) asserting a well-formed response. Very cheap given
  `kodi_client.py` already does raw JSON-RPC calls - catches "this whole namespace
  silently stopped responding" regressions that a single `Ping` can't.
- **Web-interface reachability** — a basic check that the HTML UI served by the
  webserver responds, as a separate surface from raw JSON-RPC.

## Library CRUD across all three library types

- Today's plan only covers video. Run the same add-source → scan → browse → edit
  metadata → remove-item → remove-source round trip against music and picture
  libraries too - one parametrized scenario, three library backends.
- **Playlist / smart-playlist round trip** — create, populate, play-from-playlist,
  delete.
- **Watched-state and resume-point persistence round trip**.

## Playback matrix

Once the fixture media set from `TEST_BACKLOG.md`'s playback suite exists, turn it
into a parametrized matrix instead of one clip with one assertion:

- Container/codec combinations.
- Subtitle rendering (burned-in vs. overlay).
- Audio passthrough on/off.
- Transport controls (pause/seek/speed-change/stop).
- Gapless / playlist item-to-item transition.

## Add-on lifecycle, generically

- **Install/enable/disable/uninstall smoke** against a trivial local-repo add-on of
  each type (script, skin, PVR client) - not chasing one install-hang bug, just "does
  the install/uninstall path work at all for this add-on class."
- **Add-on settings dialog round trip** — open, change a value, persist.

## Settings breadth

- `Settings.GetSettings()` full dump + one round-trip per settings *category*
  (video, audio, network, PVR, …), not just `lookandfeel`. Cheap given the mechanism
  already exists in `test_settings.py` - just needs parametrizing.

## Multi-profile

- Create a second profile, switch to it, confirm isolated userdata/library state,
  switch back.

## Skins

- Switch away from Estuary to another bundled skin, confirm Home is still reachable
  and the screenshot isn't blank. Broadens the existing screenshot check from "does
  Estuary render" to "does the skin engine render at all," independent of any one
  skin's bugs.

## Power/idle

- Screensaver engages after a configured (short, test-only) idle timeout and
  dismisses on input.

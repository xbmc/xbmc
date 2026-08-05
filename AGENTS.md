# Notes for contributors and coding agents

Short, cited notes about parts of Kodi whose behaviour is **not obvious from the public
interface** — the kind of thing you currently only learn by reading the implementation for a
few hours.

This file is deliberately tool-neutral. `AGENTS.md` is the convention several coding agents
look for, but nothing here is agent-specific: it is ordinary developer documentation that
happens to be structured for quick lookup.

## What belongs here

Non-obvious architecture, invariants and contracts. A note earns its place if a competent
contributor could reasonably get it wrong from reading the headers alone.

Every note must:

- **cite the file and symbol** — an uncited claim is a rumour;
- **state the version it was verified against** — Kodi moves, and a stale note is worse than
  no note;
- **be neutral** — describe behaviour, not opinions about it.

## What does not belong here

- Build prerequisites, tool versions and setup steps — those live in `docs/README.*.md`.
- Code style — that is `docs/CODE_GUIDELINES.md`.
- Bug reports and suspected defects — those are issues, with a repro.
- Anything specific to one contributor, machine, network or media library.

Notes are grouped by subsystem. Only the JSON-RPC and video-database areas are covered so
far; extending it to other subsystems is welcome.

---

## JSON-RPC

### The service description is generated; the JSON files are the source of truth

Editing `ServiceDescription.h` has no effect — it is generated at build time.

- Source of truth: `xbmc/interfaces/json-rpc/schema/methods.json`, `types.json`,
  `notifications.json`, `version.txt`.
- `JsonSchemaBuilder` compiles those into `ServiceDescription.h` (see
  `xbmc/interfaces/json-rpc/schema/CMakeLists.txt`). `CJSONRPC::Initialize()` then walks the
  generated `JSONRPC_SERVICE_TYPES` / `_METHODS` / `_NOTIFICATIONS` arrays.
- Validation lives in `xbmc/interfaces/json-rpc/JSONServiceDescription.cpp`.
- `JSONRPC.Introspect` serves the description at runtime, so the schema dialect is
  effectively part of the public API.

The dialect is **JSON Schema draft-03** (note `extends` and `divisibleBy`, both removed in
draft-04; `required` is a boolean on the property rather than an array on the parent; `enums`
is plural). That is not an idiosyncrasy — the layer was introduced in `50a481e333`
(2011-01-30), and draft-03 was the current draft at the time.

*Verified against 22.0-BETA1.*

### `schema/version.txt` is the API contract gate

Any change to the API surface is expected to bump `JSONRPC_VERSION`, and reviewers ask for
it. Because every API change touches this one line, it is also the file most likely to
conflict on a long-lived branch — re-derive the value from current master rather than keeping
the number a branch was opened with.

*Verified against 22.0-BETA1.*

### `Files.*` cannot browse arbitrary paths

Every `Files.*` call is gated by `CFileUtils::RemoteAccessAllowed()`
(`xbmc/utils/FileUtils.cpp`), invoked from `xbmc/interfaces/json-rpc/FileOperations.cpp`. A
path is permitted only if it begins with one of a fixed prefix list (`videodb://`,
`musicdb://`, `library://video`, `library://music`, `sources://video`, `plugin://`,
`upnp://`, `special://skin`, `special://profile/addon_data`, the configured playlists path,
and a few others), or resolves to a configured media source that is unlocked and has
`m_allowSharing` set, or to an auto-mounted removable drive.

Two consequences worth knowing: a source with sharing disabled is invisible to the API even
though it works in the UI; and refusal is reported as `InvalidParams`, so "access denied" is
indistinguishable from a malformed request. The underlying cause is logged separately by
`XFILE::CDirectory::GetDirectory`.

*Verified against 22.0-BETA1.*

### `Player.Open` is asynchronous in its generic path, synchronous in its PVR branches

`CPlayerOperations::Open` dispatches on the shape of `item`, and the branches differ in
whether they can report failure at all.

The PVR branches call playback directly and can fail:

```cpp
if (!CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayMedia(recItem))
    return FailedToExecute;
```

The generic path (`movieid`, `episodeid`, `file`, playlists) posts a message and returns
before playback is attempted:

```cpp
CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l),
                                           playername);
```

`PostMsg` is fire-and-forget, so nothing that fails after that point can reach the caller.
When adding a method that triggers playback, choose the model deliberately — returning a
success status from an asynchronous post is only honest if the caller has another way to
learn the outcome.

*Verified against 22.0-BETA1.*

### `error.data` is populated for `InvalidParams` only

`CJSONRPC::BuildResponse` attaches the optional JSON-RPC `error.data` member for exactly one
status, `InvalidParams`, where it carries structured schema-validation detail. Every other
status returns bare `code` and `message`, so clients have nothing machine-readable to branch
on beyond the code itself.

*Verified against 22.0-BETA1.*

### The layer has no automated tests, and handlers resist unit testing

`cmake/treedata/common/tests.txt` has no `xbmc/interfaces/json-rpc` entry. Handlers take
`(method, transport, client, parameterObject, result)` and reach immediately into
`CServiceBroker`, a database or the application messenger, so there is no seam to test
against without standing up most of the application.

The workable pattern is to hoist parameter parsing and validation into a dependency-free
function taking a `CVariant` and returning a status, and test that. Note that a helper placed
in an anonymous namespace has internal linkage and cannot be reached by a test at all; if it
is worth testing, it needs a header declaration.

*Verified against 22.0-BETA1.*

---

## Video and music databases

### A `false` return from `Get*Info()` does not mean "no such row"

`CVideoDatabase::GetMovieInfo()` and its siblings return `false` for at least four distinct
situations: the database is not open, the row is genuinely absent, the SQL query failed, or
an exception was caught. Callers cannot tell them apart.

This matters whenever code wants to report a trustworthy "not found" — treating every `false`
as absence means a transient database fault is reported as a deleted item. The same shape
applies to `CMusicDatabase::GetAlbum()` / `GetSong()` / `GetArtist()`.

*Verified against 22.0-BETA1.*

### There is no id-based existence check

`CMusicDatabase` provides `GetArtistExists(int)` and nothing equivalent for albums or songs.
`CVideoDatabase` provides `HasMovieInfo` / `HasTvShowInfo` / `HasEpisodeInfo` /
`HasMusicVideoInfo`, but all four take a **file path**, not a database id.

`CVideoDatabase::GetVideoItemTitle(VideoDbContentType, int)` looks like a generic id-to-title
lookup but handles only `MOVIES`; every other content type returns an empty string,
indistinguishable from "exists but has no title". It is not usable as an existence probe.

*Verified against 22.0-BETA1.*

### Library ids are reused after deletion

`movie`, `tvshow` and `episode` declare their primary key as `integer primary key` **without**
`AUTOINCREMENT`, and there is no `sqlite_sequence` table. In SQLite the next rowid is
therefore `max(rowid)+1`, so deleting the highest-numbered row and inserting another reuses
the id.

An external reference to a library id can consequently be valid, dangling, or **recycled** —
resolving successfully to a different item, with no error. `uniqueid` / `imdbnumber` are
readable properties and settable via `Media.UniqueID.Set`, but no method looks anything up by
them, so the only retrieval handle is the reusable rowid. `VideoLibrary.OnRemove` lets a
connected client invalidate proactively; a client that was offline during the removal has no
way to find out.

Worth designing around when handing out references intended to outlive a session.

*Verified against 22.0-BETA1.*

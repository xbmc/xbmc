# JSON-RPC API changelog

The version reported by `JSONRPC.Version` and carried in `openrpc.json` and
`asyncapi.json`. It moves independently of Kodi's own version.

This changelog starts at version 14. Earlier versions were not tracked here;
for those, the commit history of `xbmc/interfaces/json-rpc/` is the record.

## 14.0.0

**A breaking release.** A client written against version 13 is not guaranteed
to work unchanged. [MIGRATING-v13-to-v14.md](MIGRATING-v13-to-v14.md) covers
every break below, with what to do about each.

Everything here is new since **13.5.0, the version Kodi 21 (Omega) shipped**,
the last version delivered in a stable release. The Kodi 22 pre-releases
carried 13.8.0 (22.0a1, 22.0a2), 13.9.0 (22.0a3), 13.11.0 (22.0b1) and
13.200.0 (22.0b2). A client tested against 22.0b2 already has two of the
breaks: `NotFound` for a library id no item has, with `Unavailable` for an
unreachable `Player.Open` item, and BCP 47 stream languages. The rest are new
in 14.

### Breaking

- `WriteSetting` is a permission. `Settings.SetSettingValue`,
  `Settings.ResetSettingValue` and `Settings.SetSkinSettingValue` have declared
  it since version 6, but it was never one, and an unrecognised name gated its
  method at `ReadData`; an HTTP `GET` client, which holds `ReadData` alone,
  could change any setting. Those three methods and `Settings.SetLevel` now
  need `WriteSetting`, which every client holds except HTTP `GET`, and
  `JSONRPC.Permission` reports it. A permission name the service description
  does not know is an error, not `ReadData`.
- `JSONRPC.Introspect` answers in JSON Schema 2020-12 instead of draft-03.
  `extends` becomes `allOf`, a `$ref` is a JSON pointer, `enums` becomes
  `enum`, requiredness moves from a boolean on each property to an array on
  the containing object, and a parameter is a content descriptor wrapping its
  schema. Tuple-form `items`, `additionalItems`, `divisibleBy` and the boolean
  forms of `exclusiveMinimum`/`exclusiveMaximum` are gone.
- A PVR channel reports its client-side identifier as `channeluid`, not
  `uniqueid`. A broadcast and a recording already used that name. `uniqueid`
  on a library item is unrelated and unchanged.
- `Playlist.Add` and `Playlist.Insert` return an object naming what they could
  not add, in place of the `"OK"` string.
- `Player.GetProperties` reports `currentaudiostream`, `currentvideostream`
  and `currentsubtitle` as `null` when nothing is selected, in place of an
  empty object.
- `Files.GetDirectory` browses the directory it was given. A folder that
  matches a library item keeps its own path and comes back as
  `"filetype": "directory"`, where version 13 replaced it with the item and
  returned `"filetype": "file"` naming the video inside it. Entries for files
  are unchanged.
- `Files.GetDirectory` honours the requested `properties` under
  `"media": "files"`. 13.201.0 answers them through the video window's own
  loader, which can still replace a matched folder; 14 answers them per entry
  and keeps every folder a folder. A request naming only file properties
  (`file`, `filetype`, `label`, `mimetype`, `size`, `lastmodified`) still gets
  a plain listing and consults no library.
- `Files.GetDirectory` fills in tv show details for a show's folder. Version 13
  resolved movies, episodes and music videos but never shows, so a show folder
  came back bare even under `"media": "video"`.
- Calls that answered `InvalidParams` (-32602) for something that was named
  correctly but could not be provided answer `NotFound` (-32098),
  `Unavailable` (-32097), `AccessDenied` (-32096) or `InternalError` (-32603).
  A library id no item has is `NotFound` on every `AudioLibrary` and
  `VideoLibrary` `Get*Details`, `Set*Details` and `Refresh*` method, and a
  `Player.Open` item that cannot be reached is `Unavailable`; both since
  13.200.0. New in 14: `Files.GetDirectory`, `Files.GetFileDetails`,
  `Files.SetFileDetails`, `Files.PrepareDownload`, `Files.Download`,
  `Player.Open`, `VideoLibrary.Scan`, `VideoLibrary.Clean` and
  `AudioLibrary.GetArtistDetails`. A lookup by library id that fails in the
  database is `InternalError` on every one of those methods and on
  `Files.SetFileDetails`; version 13 reported it as `NotFound`, or as
  `InvalidParams` on `Files.SetFileDetails`.
- Every `PVR` method that names a channel, channel group, broadcast, timer,
  recording or provider by id answers `NotFound` (-32098) for an id nothing
  has, in place of `InvalidParams`.
- A `playerid` names a player, however that player was started, and a
  `Player` method answers `Unavailable` (-32097) for a player that is not
  running. Version 13 accepted an idle `playerid` when the playlist in use
  matched it and answered defaults. Every `Player` method also takes a
  `playlistid` in place of `playerid`, naming the playlist a running player is
  working through; giving both is `InvalidParams`.
- The `player` object of every `Player.On*` notification and each entry of
  `Player.GetActivePlayers` carry `playlistid`, and their `playerid` is the
  player's own: a disc played through the music playlist is `playerid` 1 on
  both, where version 13 announced it as 0.
- `Settings.GetSettingValue`, `Settings.SetSettingValue` and
  `Settings.ResetSettingValue` answer `NotFound` for a setting that does not
  exist, and the two writers answer `Unavailable` for one disabled by its
  dependencies. All three answered `InvalidParams` for both, and for a hidden
  setting. A hidden setting is now read and written like any other;
  `Settings.GetSettings` and the listings above it still filter on visibility.
- Stream languages are BCP 47 language tags, not ISO 639-2/B. `language` on
  `Player.Audio.Stream`, `Player.Video.Stream` and `Player.Subtitle` reads
  `en` where version 13 sent `eng`, with a region when the player knows one,
  `en-AU`. Inside a library item's `streamdetails` it changes the same way but
  never carries a region. An empty value still means the stream declared none;
  a value Kodi does not recognise passes through unchanged. Since 13.200.0.

### Deprecated

**Everything deprecated in 14 is removed in 15.**

Anything deprecated carries `"deprecated": true` on its method or its schema,
reported by `JSONRPC.Introspect` even when a client asks for no descriptions
and in `openrpc.json`. The description names the replacement.

- `XBMC.GetInfoLabels` and `XBMC.GetInfoBooleans`, superseded by
  `GUI.GetInfoLabels` and `GUI.GetInfoBooleans`. The old names still work and
  are served by the same implementation.
- `seasonnum` and `episodenum` on `PVR.Details.Broadcast`, superseded by
  `season` and `episode`. Deprecated in prose since 13.6.0; 14 is the first
  version to carry the annotation.
- `VideoLibrary.RefreshMovie`, `VideoLibrary.RefreshTVShow`,
  `VideoLibrary.RefreshEpisode` and `VideoLibrary.RefreshMusicVideo`,
  superseded by `VideoLibrary.Refresh`. The old names still work and are served
  by the same implementation, but the id moves inside an `item` object, so this
  one is not only a rename at the call site.

### Added

**Methods**

- `Settings.SetSettingValue` takes `confirmed`, which answers in advance the
  keep-this-mode prompt a display mode change raises. Without it the write
  blocks on the prompt and the mode reverts when nobody answers.
- `Application.SetLogLevel` - sets the log level and which components have
  extra logging, and answers with what is in force. Either part can be left
  out to keep its value.
- `AudioLibrary.RefreshAlbum` and `AudioLibrary.RefreshArtist` - refresh the
  additional information for an album or an artist.
- `AudioLibrary.SetInfoProvider` - assigns an information provider (scraper)
  to one artist or album, to every item a `musicdb://` view lists, or as the
  default, as the "Set information provider" dialog does. Music binds a
  scraper to library items rather than to a source path, which is why this
  is not `VideoLibrary.SetSourceContent` in another key.
- `Database.GetDatabaseName` - the database name in use for a given type,
  with the new `Database.Type`.
- `GUI.GetInfoLabels` and `GUI.GetInfoBooleans` - the unbranded names for the
  two deprecated `XBMC.*` methods.
- `GUI.TakeScreenshot` - takes a screenshot into the configured folder and
  answers with the `special://screenshots` path of each file it wrote, once
  encoded. `content` is `composite`, `video` or `both`; `target` names the
  file in place of the next `screenshotNNNNN.png`. `Files.PrepareDownload`
  serves the result through the web server's `/vfs/` endpoint. With no folder
  configured it answers `Unavailable`.
- `GUI.DeleteScreenshots` - deletes one screenshot or clears the folder. Off
  unless `<jsonrpc><allowscreenshotdeletion>true</allowscreenshotdeletion></jsonrpc>`
  is set in `advancedsettings.xml`; answers `Unavailable` while off.
  `Files.GetDirectory` lists `special://screenshots`.
- `PVR.GetBroadcastsByChannelGroup` - the programme of every channel of a
  channel group within a time range, answered per channel.
- `PVR.GetPlayableBroadcasts` - the playable broadcasts of a channel within a
  time range, for catchup availability.
- `PVR.GetProviders` and `PVR.GetProviderDetails`, with `PVR.Details.Provider`,
  `PVR.Fields.Provider` and `PVR.Provider.Type`.
- `Player.GetChapters` - the chapters of the playing item, with
  `Player.Chapter`.
- `Playlist.SetShuffle` and `Playlist.SetRepeat`.
- `Settings.GetLevel` and `Settings.SetLevel` - the setting level in force,
  the one the settings window shows. `Settings.SetLevel` answers
  `AccessDenied` when the profile's settings lock refuses the level, and a
  settings window that is open redraws for the new level. The `level`
  parameter of `Settings.GetSections`, `Settings.GetCategories` and
  `Settings.GetSettings` is a filter over the tree that defaults to
  `standard` whatever the level in force, and now says so.
- `VideoLibrary.Refresh` - refreshes the library item its `item` parameter
  names, in place of the four deprecated per-type methods. A movie set and a
  season can be refreshed for the first time.
- `VideoLibrary.SetSourceContent` - assigns a content type and scraper to a
  video source path, as the "Set content" dialog does.

**Notifications**

- `GUI.OnSkinLoaded`, `GUI.OnSkinLoadFailed` and `GUI.OnSkinUnloading`.
- `Player.OnPlaybackFailed`, raised when playback was asked for and did not
  happen.
- `Playlist.OnPropertyChanged`, raised when a playlist's shuffle or repeat
  state changes.
- `Settings.OnLevelChanged`, raised when the setting level in force changes,
  including when the profile's settings lock lowers it. `Settings` is a new
  notification namespace; a client receives it unless it has narrowed its
  subscription with `JSONRPC.SetConfiguration`.

**Properties and types**

- An error taxonomy in `JSONRPC.Introspect`, under `errors`: every status a
  call can fail with, its code, its message, and whether it carries `data`.
  `JSONRPC.Introspect` accepts `"error"` as a filter type.
- `errors` on every method in `JSONRPC.Introspect` and `openrpc.json`: the
  errors that method's implementation can return, beyond the ones any request
  can receive. Derived from the handler source.
- `AccessDenied` (-32096), for a path outside every source shared for remote
  access, or a setting level the profile's settings lock keeps.
- `loglevel` on `Application.GetProperties`, with `Application.LogLevel`,
  `Application.LogLevel.Value` and `Application.LogComponent`: the level in
  force and every log component this build knows, by its own name, with
  whether each is enabled.
- `ready` on `GUI.GetProperties`: true once the interface has finished
  starting and a skin is loaded with its first window active, false while Kodi
  is still starting or a skin is reloading. The web server answering proves
  only that the JSON-RPC service is up, and `GUI.OnSkinLoaded` reaches only a
  client holding a socket open; this is the answer a client that polls can
  read.
- `shuffled` and `repeat` on `Playlist.GetProperties`.
- `Playlist.AddResult` and `Playlist.UnresolvedItem`.
- `stationname` on `List.Item.Base`, the radio station serving an internet
  stream. `episodename` and `episodepart` are also newly requestable in
  `List.Fields.All`.
- `codec` on `Player.Subtitle`; `bitspersample` on `Player.Audio.Stream`.
- `starttime` and `endtime` on `PVR.GetBroadcasts`, which bound the answer
  to the broadcasts overlapping that range. Without them it answers as it
  always did.
- `season` and `episode` on `PVR.Details.Broadcast`, replacing `seasonnum`
  and `episodenum`.
- `parentalratingcode`, `parentalratingicon` and `parentalratingsource` on
  `PVR.Details.Broadcast`; those three plus `parentalrating` on
  `PVR.Details.Recording`.
- `status` and `trailer` on `Video.Details.TVShow`.
- `lastlibrarycheck` on `Textures.Details.Texture`.
- `games` on `Files.Media`, so `Files.GetSources` reaches the game sources.
- `Notifications.Library.Audio.Type` and `Notifications.Library.Video.Type`,
  the media types carried by the library notifications, named as types rather
  than repeated inline.

### Changed

- `Settings.GetSections`, `Settings.GetCategories` and `Settings.GetSettings`
  answer with the `level` they filtered at, which is the one the call asked
  for, default `standard`, not the level in force.
- `Configuration.Notifications` declares `Info`, `Sources` and `Settings`.
  `JSONRPC.GetConfiguration` reported the first two, which the type forbade.
- `Player.OnPropertyChanged` carries the members `Player.Property.Value`
  declares, rather than a subset.
- PVR image properties (`icon`, `thumbnail`, `parentalratingicon`, and a
  recording's `art`) are URLs the web server's `/image/` endpoint can serve,
  rather than paths only the local machine could read.
- A PVR channel keeps its own logo in `icon` and `thumbnail` when the
  programme airing on it has its own artwork. The programme's artwork remains
  available under `broadcastnow`.
- Every cast member of a PVR item carries a `role` and an `order`, which
  `Video.Cast` requires.
- `XBMC.GetInfoBooleans` and `GUI.GetInfoBooleans` describe their return as an
  object of booleans, which is what has always been answered.
- `Application.Property.Name` lists `volume` once. It was listed twice.
- `Textures.Details.Texture` declares `textureid` required.
- The header of a `JSONRPC.Introspect` answer names Kodi. `id` is
  `https://kodi.tv/jsonrpc/ServiceDescription.json` and `description` is
  "JSON-RPC API of Kodi"; both said XBMC.
- `Player.Open` with `item.path` plays a directory that holds no pictures as
  a playlist of its video and audio files. A directory with pictures is a
  slideshow, as before; `random` applies only there.
- `VideoLibrary.SetTVShowDetails` accepts `trailer`.

### Fixed

- Announcements are not blocked while the TCP server is busy, and each
  request runs on its connection's own thread, so a modal dialog stalls no
  other client.
- A failing send gives up instead of spinning.
- The JSON-RPC methods are registered before anything can call them.
- `Settings.SetSettingValue` answers `InvalidParams` for a value the setting
  does not offer and `Unavailable` for a change a handler declined, a display
  mode not kept for one. Both were a `false` result.
- `Video.Streams` declares the `source` and `version` every stream carries,
  and the `flags` bitmask on audio and subtitle streams, all of which the
  serializer has emitted since the fields were added.
- `JSONRPC.SetConfiguration` keeps every namespace the caller does not name.
  It dropped `PVR`, `Info` and `Sources` on every call and kept `Application`
  by the state of `Other`. The three are declared in its parameter.
- `Playlist.Clear` resets the playlist position that indexed the cleared
  items.
- `Player.GetItem` reports AirPlay cover art, and live stream metadata for a
  playing PVR radio channel.
- `Player.GetItem` and `Files.GetFileDetails` keep the metadata of an item the
  library does not hold, an add-on's typically, once it has been played.
- `file` agrees with `filetype` for a movie with versions or extras.
- `Files.GetFileDetails` reports the `file` and `filetype` its result type
  requires.
- `VideoLibrary.SetTVShowDetails` applies `playcount` and `lastplayed` to the
  show's episodes.
- `VideoLibrary.Clean` honours its `directory` parameter.
- `Playlist.Add` keeps an album's tracks together when several albums are
  added at once.
- A hidden subtitle keeps its selection when its stream is closed.
- `Files.PrepareDownload` reports the scheme the client reached Kodi by, so
  `protocol` can be `https`. It was always `http`.
- `PVR.Details.Broadcast` declares `imdbnumber` as a string, and it and
  `PVR.Details.Recording` declare `genre` as an array of strings, which is
  what each has always sent.
- The `broadcastnow` and `broadcastnext` sub-objects of a PVR channel carry
  the `label` their type requires and answer with the fields
  `PVR.Fields.Broadcast` declares, so `hastimer`, `hastimerrule`,
  `hasreminder`, `hasrecording`, `recording` and `recordingid` are readable
  inside them; the undeclared `channeluid`, `filenameandpath`, `serieslink`
  and `titleextrainfo` no longer appear.
- A broadcast reports a `starttime` or `endtime` that falls on or after
  2038-01-19, in place of a date in the past.
- The video streams of a library item declare `stereomode`, `language` and
  `hdrdetail`, which the serializer has always sent.

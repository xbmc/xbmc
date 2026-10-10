/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//! \brief The keys of list item properties that one part of Kodi sets and another reads back.
//! Property keys are case-insensitive.
namespace KODI::ITEM::PROPERTY
{

// Playback
inline constexpr char ORIGINAL_LISTITEM_URL[] = "original_listitem_url";
inline constexpr char ITEM_START[] = "item_start";
inline constexpr char START_PERCENT[] = "StartPercent";
inline constexpr char IS_PLAYABLE[] = "IsPlayable";
inline constexpr char FORCE_PLAYLIST_SELECTION[] = "force_playlist_selection";
inline constexpr char BLURAY_PLAYLIST[] = "bluray_playlist";
inline constexpr char UPDATE_STREAM_DETAILS[] = "update_stream_details";
inline constexpr char STOPPED_BEFORE_END[] = "stopped_before_end";
inline constexpr char NO_MAIN_TITLE[] = "no_main_title";
inline constexpr char NEW_STACK_PATH[] = "new_stack_path";
inline constexpr char NEW_PLAYLIST_PATH[] = "new_playlist_path";
inline constexpr char REPLACED_FILE_ID[] = "replaced_file_id";
inline constexpr char PLAYCOUNT_INCREMENTED[] = "playcount_incremented";
inline constexpr char SAVED_PLAYER_STATE[] = "savedplayerstate";
inline constexpr char CHECK_AUTOPLAY_NEXT_ITEM[] = "CheckAutoPlayNextItem";
inline constexpr char EPG_PLAYLIST_ITEM[] = "epg_playlist_item";
inline constexpr char AUDIOBOOK_BOOKMARK[] = "audiobook_bookmark";
inline constexpr char CUE_LOAD_INFORMATION[] = "cueloadinformation";

// Listings
inline constexpr char SET_VIDEODB_DETAILS[] = "set_videodb_details";
inline constexpr char LIBRARY_ART_FILLED[] = "libraryartfilled";
inline constexpr char ICON_NEVER_OVERLAY[] = "icon_never_overlay";
inline constexpr char IS_VIDEO_FOLDER[] = "IsVideoFolder";
inline constexpr char IS_HYBRID_FOLDER[] = "IsHybridFolder";
inline constexpr char IS_HTTP_DIRECTORY[] = "IsHTTPDirectory";
inline constexpr char IS_STACKED[] = "isstacked";
inline constexpr char PARENT_PATH[] = "ParentPath";
inline constexpr char CACHE_FILENAME[] = "cachefilename";
inline constexpr char LIBRARY_FILTER[] = "library.filter";
inline constexpr char LIBRARY_SMARTPLAYLIST[] = "library.smartplaylist";
inline constexpr char WATCHED_MODE[] = "watchedmode";
inline constexpr char PLAYLIST_POSITION[] = "playlistposition";
inline constexpr char DEVICE_PATH[] = "device_path";
inline constexpr char HIDE_ADD_REMOVE_FAVOURITE[] = "hide_add_remove_favourite";
inline constexpr char TIMELINE_INDEX[] = "TimelineIndex";

// Music and video
inline constexpr char ARTIST_MUSICID[] = "artist_musicid";
inline constexpr char ALBUM_MUSICID[] = "album_musicid";
inline constexpr char MUSICVIDEO_MEDIA_TYPE[] = "musicvideomediatype";
inline constexpr char CUSTOM_TITLE[] = "customtitle";
inline constexpr char SET_FOLDER_THUMB[] = "set_folder_thumb";
inline constexpr char EPISODES_SHOW_PLOT[] = "episodes_show_plot";
inline constexpr char EPISODES_SPECIALS[] = "episodes_specials";

// Episode counts of a show or season
inline constexpr char TOTAL_SEASONS[] = "totalseasons";
inline constexpr char TOTAL_EPISODES[] = "totalepisodes";
//! The episodes the watched filter shows
inline constexpr char NUM_EPISODES[] = "numepisodes";
inline constexpr char WATCHED_EPISODES[] = "watchedepisodes";
inline constexpr char UNWATCHED_EPISODES[] = "unwatchedepisodes";
inline constexpr char IN_PROGRESS_EPISODES[] = "inprogressepisodes";

// Add-on listings
inline constexpr char ADDON_ID[] = "Addon.ID";
inline constexpr char ADDON_NAME[] = "Addon.Name";
inline constexpr char ADDON_STATUS[] = "Addon.Status";
inline constexpr char ADDON_HAS_UPDATE[] = "Addon.HasUpdate";
inline constexpr char ADDON_VALID_UPDATE_ORIGIN[] = "Addon.ValidUpdateOrigin";
inline constexpr char ADDON_VALID_UPDATE_VERSION[] = "Addon.ValidUpdateVersion";
inline constexpr char ADDON_DOWNLOADING[] = "Addon.Downloading";

} // namespace KODI::ITEM::PROPERTY

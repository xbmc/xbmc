/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/NumericInputMode.h"
#include "interfaces/AnnouncementMessages.h"
#include "interfaces/IAnnouncer.h"
#include "media/MediaStreams.h"
#include "media/MediaType.h"
#include "playlists/PlayListTypes.h"
#include "utils/Variant.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>

class CFileItem;
enum class SettingLevel;

/*!
 * \brief What an announcement says, as the sender knew it when it was raised.
 *
 * A listener acts on the event and never on live state: delivery happens later, on the
 * announcement manager's own thread. An item an event carries is a copy taken when it was
 * announced, and nothing changes it.
 */
namespace ANNOUNCEMENT
{

namespace EVENT::PLAYER
{
struct Play
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PLAY;

  std::shared_ptr<const CFileItem> item;
  int speed{1};
  KODI::MEDIA::Streams streams{};
};

struct AVStart
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_AV_START;

  std::shared_ptr<const CFileItem> item;
  KODI::MEDIA::Streams streams{};
};

struct AVChange
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_AV_CHANGE;

  std::shared_ptr<const CFileItem> item;
  KODI::MEDIA::Streams streams{};
};

struct Pause
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PAUSE;

  std::shared_ptr<const CFileItem> item;
  KODI::MEDIA::Streams streams{};
};

struct Resume
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_RESUME;

  std::shared_ptr<const CFileItem> item;
  KODI::MEDIA::Streams streams{};
};

struct SpeedChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SPEED_CHANGED;

  std::shared_ptr<const CFileItem> item;
  int speed{1};
  KODI::MEDIA::Streams streams{};
};

struct Seek
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SEEK;

  std::shared_ptr<const CFileItem> item;
  int speed{1};
  std::chrono::milliseconds time{0};
  std::chrono::milliseconds seekOffset{0};
  KODI::MEDIA::Streams streams{};
};

struct Stop
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_STOP;

  std::shared_ptr<const CFileItem> item;
  //! The playback reached its end, rather than being stopped.
  bool end{false};
  //! Given for the slideshow only.
  std::optional<KODI::MEDIA::Streams> streams;
};

struct PlaybackFailed
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PLAYBACK_FAILED;

  std::shared_ptr<const CFileItem> item;
  //! Empty when the player gave no reason.
  std::optional<KODI::PLAYLIST::FailReason> reason;
};

//! The properties of the player that changed; only those given changed.
struct PropertiesChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PROPERTIES_CHANGED;

  std::optional<KODI::MEDIA::Streams> streams{};
  std::optional<int> volume{};
  std::optional<bool> muted{};
  std::optional<bool> partyMode{};
  std::optional<bool> subtitleEnabled{};
  //! The streams as JSON-RPC describes them.
  std::optional<CVariant> currentSubtitle{};
  std::optional<CVariant> currentAudioStream{};
  std::optional<CVariant> currentVideoStream{};
};

struct ContentGeometryChange
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_CONTENT_GEOMETRY_CHANGE;

  //! The geometry as JSON-RPC describes it.
  CVariant geometry;
  KODI::MEDIA::Streams streams{};
};

struct Commercial
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_COMMERCIAL;

  //! How long the commercial lasts, or where it ends, as MM:SS.
  std::string time;
};

struct ToggleSkipCommercials
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_TOGGLE_SKIP_COMMERCIALS;

  bool skip{false};
};

struct ProcessInfo
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PROCESS_INFO;
};

struct Menu
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_MENU;
};

struct BlurayMenuError
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_BLURAY_MENU_ERROR;
};

struct BlurayEncryptedError
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_BLURAY_ENCRYPTED_ERROR;
};

//! The source delivers data more slowly than playback needs.
struct SourceSlow
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::SOURCE_SLOW;
};

//! Playback is held while the audio chain acquires a new format. It resumes when the chain is
//! reported ready or the time is up.
struct AudioFormatHold
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_AUDIO_FORMAT_HOLD;

  bool realtime{false};
  bool passthrough{false};
  //! The longest the hold lasts, in seconds.
  double timeout{0.0};
};
//! Whether the item is announced as a picture rather than as a video, song or channel.
bool IsPicture(const CFileItem* item);
} // namespace EVENT::PLAYER

struct PlayerEvent : std::variant<EVENT::PLAYER::Play,
                                  EVENT::PLAYER::AVStart,
                                  EVENT::PLAYER::AVChange,
                                  EVENT::PLAYER::Pause,
                                  EVENT::PLAYER::Resume,
                                  EVENT::PLAYER::SpeedChanged,
                                  EVENT::PLAYER::Seek,
                                  EVENT::PLAYER::Stop,
                                  EVENT::PLAYER::PlaybackFailed,
                                  EVENT::PLAYER::PropertiesChanged,
                                  EVENT::PLAYER::ContentGeometryChange,
                                  EVENT::PLAYER::Commercial,
                                  EVENT::PLAYER::ToggleSkipCommercials,
                                  EVENT::PLAYER::ProcessInfo,
                                  EVENT::PLAYER::Menu,
                                  EVENT::PLAYER::BlurayMenuError,
                                  EVENT::PLAYER::BlurayEncryptedError,
                                  EVENT::PLAYER::SourceSlow,
                                  EVENT::PLAYER::AudioFormatHold>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Player;
};

//! A playlist event's list is a playlist, or empty for the slideshow's pictures.
namespace EVENT::PLAYLIST
{
struct Add
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_ADD;

  std::optional<KODI::PLAYLIST::Type> playList;
  int position{-1};
  std::shared_ptr<const CFileItem> item;
};

struct Remove
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_REMOVE;

  std::optional<KODI::PLAYLIST::Type> playList;
  int position{-1};
};

struct Clear
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_CLEAR;

  std::optional<KODI::PLAYLIST::Type> playList;
};

//! The properties of a playlist that changed; only those given changed.
struct PropertiesChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PROPERTIES_CHANGED;

  std::optional<KODI::PLAYLIST::Type> playList;
  std::optional<bool> shuffled;
  std::optional<KODI::PLAYLIST::Repeat> repeat;
};
} // namespace EVENT::PLAYLIST

struct PlaylistEvent : std::variant<EVENT::PLAYLIST::Add,
                                    EVENT::PLAYLIST::Remove,
                                    EVENT::PLAYLIST::Clear,
                                    EVENT::PLAYLIST::PropertiesChanged>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Playlist;
};

namespace EVENT::GUI
{
struct ScreensaverActivated
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SCREENSAVER_ACTIVATED;
};

struct ScreensaverDeactivated
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SCREENSAVER_DEACTIVATED;

  //! A power down or suspend follows, so the deactivation may be ignored.
  bool shuttingDown{false};
};

struct DPMSActivated
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_DPMS_ACTIVATED;
};

struct DPMSDeactivated
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_DPMS_DEACTIVATED;
};

struct SkinUnloading
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SKIN_UNLOADING;
};

struct SkinLoaded
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SKIN_LOADED;
};

struct SkinLoadFailed
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SKIN_LOAD_FAILED;
};

struct WindowFocused
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::WINDOW_FOCUSED;
};

struct WindowUnfocused
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::WINDOW_UNFOCUSED;
};
} // namespace EVENT::GUI

struct GUIEvent : std::variant<EVENT::GUI::ScreensaverActivated,
                               EVENT::GUI::ScreensaverDeactivated,
                               EVENT::GUI::DPMSActivated,
                               EVENT::GUI::DPMSDeactivated,
                               EVENT::GUI::SkinUnloading,
                               EVENT::GUI::SkinLoaded,
                               EVENT::GUI::SkinLoadFailed,
                               EVENT::GUI::WindowFocused,
                               EVENT::GUI::WindowUnfocused>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = GUI;
};

namespace EVENT::SYSTEM
{
struct Quit
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_QUIT;

  int exitCode{0};
};

struct Restart
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_RESTART;
};

struct Sleep
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SLEEP;
};

struct Wake
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_WAKE;
};

struct LowBattery
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_LOW_BATTERY;
};
} // namespace EVENT::SYSTEM

struct SystemEvent : std::variant<EVENT::SYSTEM::Quit,
                                  EVENT::SYSTEM::Restart,
                                  EVENT::SYSTEM::Sleep,
                                  EVENT::SYSTEM::Wake,
                                  EVENT::SYSTEM::LowBattery>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = System;
};

namespace EVENT::LIBRARY
{
struct ScanStarted
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SCAN_STARTED;
};

struct ScanFinished
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_SCAN_FINISHED;
};

struct CleanStarted
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_CLEAN_STARTED;
};

struct CleanFinished
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_CLEAN_FINISHED;
};

struct Update
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_UPDATE;

  KODI::MEDIA::TYPE type{KODI::MEDIA::TYPE::NONE};
  //! The stored type of an item no KODI::MEDIA::TYPE names, such as an actor, in place of type.
  std::string otherType{};
  int id{-1};
  //! The item itself, when the sender has it: the data JSON-RPC and Python are given then
  //! describes it under "item" in place of the type and id.
  std::shared_ptr<const CFileItem> item{};
  bool transaction{false};
  bool added{false};
  std::optional<int> playCount{};
  //! The properties that changed, as JSON-RPC describes them.
  std::optional<CVariant> properties{};
};

struct Remove
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_REMOVE;

  KODI::MEDIA::TYPE type{KODI::MEDIA::TYPE::NONE};
  int id{-1};
  bool transaction{false};
};

struct Export
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_EXPORT;

  std::optional<std::string> root{};
  std::optional<std::string> file{};
  std::optional<int> failCount{};
};

struct Refresh
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_REFRESH;
};
} // namespace EVENT::LIBRARY

struct LibraryEvent : std::variant<EVENT::LIBRARY::ScanStarted,
                                   EVENT::LIBRARY::ScanFinished,
                                   EVENT::LIBRARY::CleanStarted,
                                   EVENT::LIBRARY::CleanFinished,
                                   EVENT::LIBRARY::Update,
                                   EVENT::LIBRARY::Remove,
                                   EVENT::LIBRARY::Export,
                                   EVENT::LIBRARY::Refresh>
{
  using variant::variant;
};

struct VideoLibraryEvent : LibraryEvent
{
  using LibraryEvent::LibraryEvent;
  static constexpr AnnouncementFlag FLAG = VideoLibrary;
};

struct AudioLibraryEvent : LibraryEvent
{
  using LibraryEvent::LibraryEvent;
  static constexpr AnnouncementFlag FLAG = AudioLibrary;
};

//! Whether the event is part of a scan or clean, which announces its own end.
bool IsTransaction(const LibraryEvent& event);

namespace EVENT::INPUT
{
struct Requested
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_INPUT_REQUESTED;

  //! The numeric dialog's mode, or empty for the keyboard.
  std::optional<KODI::DIALOGS::NUMERIC_MODE> numeric{};
  //! The keyboard hides what is typed.
  bool hidden{false};
  std::optional<std::string> title{};
  std::string value{};
};

struct Finished
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_INPUT_FINISHED;
};
} // namespace EVENT::INPUT

struct InputEvent : std::variant<EVENT::INPUT::Requested, EVENT::INPUT::Finished>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Input;
};

namespace EVENT::PVR
{
//! A radio traffic announcement started or ended.
struct RadioTrafficAnnouncement
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::RDS_RADIO_TA;

  bool on{false};
};

//! The time a radio station's RDS clock gives.
struct RadioClock
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::RDS_RADIO_RTC;

  //! RFC 1123, or empty when the clock is invalid.
  std::string dateTime;
};

//! A radio traffic message channel (RDS-TMC) message.
struct RadioTrafficMessage
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::RDS_RADIO_TMC;

  std::string channel;
  uint16_t ident{0};
  unsigned int flags{0};
  uint8_t x{0};
  unsigned int y{0};
  unsigned int z{0};
};
} // namespace EVENT::PVR

struct PVREvent : std::variant<EVENT::PVR::RadioTrafficAnnouncement,
                               EVENT::PVR::RadioClock,
                               EVENT::PVR::RadioTrafficMessage>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = ANNOUNCEMENT::PVR;
};

namespace EVENT::INFO
{
//! The item the info labels describe changed.
struct Changed
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_CHANGED;
};
} // namespace EVENT::INFO

struct InfoEvent : std::variant<EVENT::INFO::Changed>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Info;
};

namespace EVENT::SOURCES
{
struct Added
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_ADDED;

  std::string path;
};

struct Removed
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_REMOVED;

  std::string path;
};

struct Updated
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_UPDATED;

  std::string path;
};
} // namespace EVENT::SOURCES

struct SourcesEvent
  : std::variant<EVENT::SOURCES::Added, EVENT::SOURCES::Removed, EVENT::SOURCES::Updated>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Sources;
};

namespace EVENT::FILES
{
struct PlayCountChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_PLAY_COUNT_CHANGED;

  std::string file;
  int playCount{0};
};

struct ResumePointChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_RESUME_POINT_CHANGED;

  std::string file;
  double position{0.0};
  double total{0.0};
};
} // namespace EVENT::FILES

struct FilesEvent : std::variant<EVENT::FILES::PlayCountChanged, EVENT::FILES::ResumePointChanged>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Files;
};

namespace EVENT::SETTINGS
{
struct LevelChanged
{
  static constexpr const char* MESSAGE = ANNOUNCEMENT::MESSAGE::ON_LEVEL_CHANGED;

  SettingLevel level{};
};
} // namespace EVENT::SETTINGS

struct SettingsEvent : std::variant<EVENT::SETTINGS::LevelChanged>
{
  using variant::variant;
  static constexpr AnnouncementFlag FLAG = Settings;
};

//! \brief An announcement whose sender names it and its data, as JSONRPC.NotifyAll sends.
struct OtherEvent
{
  static constexpr AnnouncementFlag FLAG = Other;

  std::string sender;
  std::string message;
  CVariant data;
};

struct Announcement : std::variant<PlayerEvent,
                                   PlaylistEvent,
                                   GUIEvent,
                                   SystemEvent,
                                   VideoLibraryEvent,
                                   AudioLibraryEvent,
                                   InputEvent,
                                   PVREvent,
                                   InfoEvent,
                                   SourcesEvent,
                                   FilesEvent,
                                   SettingsEvent,
                                   OtherEvent>
{
  using variant::variant;
};

AnnouncementFlag FlagOf(const Announcement& announcement);
const char* MessageOf(const Announcement& announcement);
//! \brief Who announced it: the announcement manager, or for Other whoever sent it.
const std::string& SenderOf(const Announcement& announcement);
std::shared_ptr<const CFileItem> ItemOf(const Announcement& announcement);

/*!
 * \brief The announcement with its item's library details loaded, when the item has no database
 * id and the library knows it by its path.
 */
Announcement WithLibraryDetails(Announcement announcement);

/*!
 * \brief The announcement with its item replaced by \p item.
 */
Announcement WithItem(Announcement announcement, std::shared_ptr<const CFileItem> item);

} // namespace ANNOUNCEMENT

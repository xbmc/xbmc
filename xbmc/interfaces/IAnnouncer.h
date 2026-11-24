/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace ANNOUNCEMENT
{
enum AnnouncementFlag
{
  Player = 0x001,
  Playlist = 0x002,
  GUI = 0x004,
  System = 0x008,
  VideoLibrary = 0x010,
  AudioLibrary = 0x020,
  Application = 0x040,
  Input = 0x080,
  PVR = 0x100,
  Other = 0x200,
  Info = 0x400,
  Sources = 0x800,
  Files = 0x1000,
  Settings = 0x2000
};

const auto ANNOUNCE_ALL =
    (Player | Playlist | GUI | System | VideoLibrary | AudioLibrary | Application | Input |
     ANNOUNCEMENT::PVR | Other | Info | Sources | Files | Settings);

/*!
    \brief Returns a string representation for the
    given AnnouncementFlag
    \param notification Specific AnnouncementFlag
    \return String representation of the given AnnouncementFlag
    */
inline const char* AnnouncementFlagToString(const AnnouncementFlag& notification)
{
  switch (notification)
  {
    case Player:
      return "Player";
    case Playlist:
      return "Playlist";
    case GUI:
      return "GUI";
    case System:
      return "System";
    case VideoLibrary:
      return "VideoLibrary";
    case AudioLibrary:
      return "AudioLibrary";
    case Application:
      return "Application";
    case Input:
      return "Input";
    case PVR:
      return "PVR";
    case Other:
      return "Other";
    case Info:
      return "Info";
    case Sources:
      return "Sources";
    case Files:
      return "Files";
    case Settings:
      return "Settings";
    default:
      return "Unknown";
  }
}

struct PlayerEvent;
struct PlaylistEvent;
struct GUIEvent;
struct SystemEvent;
struct LibraryEvent;
struct VideoLibraryEvent;
struct AudioLibraryEvent;
struct InputEvent;
struct PVREvent;
struct InfoEvent;
struct SourcesEvent;
struct FilesEvent;
struct SettingsEvent;
struct OtherEvent;
struct Announcement;

class IAnnouncer
{
public:
  IAnnouncer() = default;
  virtual ~IAnnouncer() = default;

  /*!
     * \brief Every announcement of the flags the announcer was added for. Each goes to the
   * handler for its flag unless this is overridden.
   */
  virtual void OnAnnouncement(const Announcement& announcement);

  virtual void OnPlayerEvent(const PlayerEvent& event) {}
  virtual void OnPlaylistEvent(const PlaylistEvent& event) {}
  virtual void OnGUIEvent(const GUIEvent& event) {}
  virtual void OnSystemEvent(const SystemEvent& event) {}
  virtual void OnVideoLibraryEvent(const VideoLibraryEvent& event) {}
  virtual void OnAudioLibraryEvent(const AudioLibraryEvent& event) {}
  virtual void OnInputEvent(const InputEvent& event) {}
  virtual void OnPVREvent(const PVREvent& event) {}
  virtual void OnInfoEvent(const InfoEvent& event) {}
  virtual void OnSourcesEvent(const SourcesEvent& event) {}
  virtual void OnFilesEvent(const FilesEvent& event) {}
  virtual void OnSettingsEvent(const SettingsEvent& event) {}
  virtual void OnOtherEvent(const OtherEvent& event) {}
};
}

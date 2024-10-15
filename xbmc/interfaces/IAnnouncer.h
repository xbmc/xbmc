/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CVariant;
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
  Sources = 0x800
};

const auto ANNOUNCE_ALL = (Player | Playlist | GUI | System | VideoLibrary | AudioLibrary |
                           Application | Input | ANNOUNCEMENT::PVR | Other | Info | Sources);

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
    default:
      return "Unknown";
  }
}

  class IAnnouncer
  {
  public:
    IAnnouncer() = default;
    virtual ~IAnnouncer() = default;
    virtual void Announce(AnnouncementFlag flag,
                          const std::string& sender,
                          const std::string& message,
                          const CVariant& data) = 0;
  };
}

#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

class CVariant;
namespace ANNOUNCEMENT
{
  enum AnnouncementFlag
  {
    Player        = 0x01,
    Playlist      = 0x02,
    GUI           = 0x04,
    System        = 0x08,
    VideoLibrary  = 0x10,
    AudioLibrary  = 0x20,
    Application   = 0x40,
    Input         = 0x80,
    Other         = 0x100
  };

  #define ANNOUNCE_ALL (Player | Playlist | GUI | System | VideoLibrary | AudioLibrary | Application | Input | Other)

  /*!
    \brief Returns a string representation for the 
    given AnnouncementFlag
    \param notification Specific AnnouncementFlag
    \return String representation of the given AnnouncementFlag
    */
  inline const char *AnnouncementFlagToString(const AnnouncementFlag &notification)
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
    case Other:
      return "Other";
    default:
      return "Unknown";
    }
  }

  class IAnnouncer
  {
  public:
    IAnnouncer() { };
    virtual ~IAnnouncer() { };
    virtual void Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) = 0;
  };
}

#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IAnnouncer.h"

namespace ANNOUNCEMENT
{
  class CAnnouncementUtils
  {
  public:
    /*!
     \brief Returns a string representation for the 
     given EAnnouncementFlag
     \param notification Specific EAnnouncementFlag
     \return String representation of the given EAnnouncementFlag
     */
    static inline const char *AnnouncementFlagToString(const EAnnouncementFlag &notification)
    {
      switch (notification)
      {
      case Player:
        return "Player";
      case GUI:
        return "GUI";
      case System:
        return "System";
      case VideoLibrary:
        return "VideoLibrary";
      case AudioLibrary:
        return "AudioLibrary";
      case Other:
        return "Other";
      default:
        return "Unknown";
      }
    }
  };
}

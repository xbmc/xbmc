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

#include "IAnnouncer.h"
#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"
#include <vector>

namespace ANNOUNCEMENT
{
  class CAnnouncementManager
  {
  public:

     class Globals
     {
     public:
       CCriticalSection m_critSection;
       std::vector<IAnnouncer *> m_announcers;
     };

    static void AddAnnouncer(IAnnouncer *listener);
    static void RemoveAnnouncer(IAnnouncer *listener);
    static void Announce(AnnouncementFlag flag, const char *sender, const char *message);
    static void Announce(AnnouncementFlag flag, const char *sender, const char *message, CVariant &data);
    static void Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item);
    static void Announce(AnnouncementFlag flag, const char *sender, const char *message, CFileItemPtr item, CVariant &data);
  private:
  };
}

XBMC_GLOBAL_REF(ANNOUNCEMENT::CAnnouncementManager::Globals,g_announcementManager_globals);

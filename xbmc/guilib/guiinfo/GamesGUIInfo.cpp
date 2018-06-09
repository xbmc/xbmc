/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
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

#include "guilib/guiinfo/GamesGUIInfo.h"

#include "FileItem.h"
#include "Util.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "games/addons/savestates/SavestateDefines.h"
#include "games/tags/GameInfoTag.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

using namespace KODI::GUILIB::GUIINFO;
using namespace KODI::GAME;
using namespace KODI::RETRO;

bool CGamesGUIInfo::InitCurrentItem(CFileItem *item)
{
  if (item && item->IsGame())
  {
    CLog::Log(LOGDEBUG, "CGamesGUIInfo::InitCurrentItem(%s)", item->GetPath().c_str());

    item->LoadGameTag();
    CGameInfoTag* tag = item->GetGameInfoTag(); // creates item if not yet set, so no nullptr checks needed

    if (tag->GetTitle().empty())
    {
      // No title in tag, show filename only
      tag->SetTitle(CUtil::GetTitleFromPath(item->GetPath()));
    }
    return true;
  }
  return false;
}

bool CGamesGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // RETROPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case RETROPLAYER_VIEWMODE:
    {
      VIEWMODE viewMode = CMediaSettings::GetInstance().GetCurrentGameSettings().ViewMode();
      value = CRetroPlayerUtils::ViewModeToDescription(viewMode);
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_DURATION:
      if (item->HasProperty(FILEITEM_PROPERTY_SAVESTATE_DURATION))
      {
        int iDuration = static_cast<long>(item->GetProperty(FILEITEM_PROPERTY_SAVESTATE_DURATION).asInteger());
        if (iDuration > 0)
        {
          value = StringUtils::SecondsToTimeString(iDuration, static_cast<TIME_FORMAT>(info.GetData4()));
          return true;
        }
      }
      break;
  }

  return false;
}

bool CGamesGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CGamesGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

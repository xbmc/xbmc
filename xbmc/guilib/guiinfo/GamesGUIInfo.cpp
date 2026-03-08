/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GamesGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI::GUILIB::GUIINFO;
using namespace KODI::GAME;
using namespace KODI::RETRO;

//! @todo Savestates were removed from v18
//#define FILEITEM_PROPERTY_SAVESTATE_DURATION  "duration"

bool CGamesGUIInfo::InitCurrentItem(CFileItem* item)
{
  if (item && item->IsGame())
  {
    CLog::Log(LOGDEBUG, "CGamesGUIInfo::InitCurrentItem({})", item->GetPath());

    item->LoadGameTag();
    CGameInfoTag* tag =
        item->GetGameInfoTag(); // creates item if not yet set, so no nullptr checks needed

    if (tag->GetTitle().empty())
    {
      // No title in tag, show filename only
      tag->SetTitle(CUtil::GetTitleFromPath(item->GetPath()));
    }
    return true;
  }
  return false;
}

bool CGamesGUIInfo::GetLabel(std::string& value,
                             const CFileItem* item,
                             int contextWindow,
                             const CGUIInfo& info,
                             std::string* fallback) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // RETROPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case RETROPLAYER_VIDEO_FILTER:
    {
      value = CMediaSettings::GetInstance().GetCurrentGameSettings().VideoFilter();
      return true;
    }
    case RETROPLAYER_STRETCH_MODE:
    {
      STRETCHMODE stretchMode =
          CMediaSettings::GetInstance().GetCurrentGameSettings().StretchMode();
      value = CRetroPlayerUtils::StretchModeToIdentifier(stretchMode);
      return true;
    }
    case RETROPLAYER_VIDEO_ROTATION:
    {
      const unsigned int rotationDegCCW =
          CMediaSettings::GetInstance().GetCurrentGameSettings().RotationDegCCW();
      value = std::to_string(rotationDegCCW);
      return true;
    }
    case RETROPLAYER_DISC_LABEL:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      if (appPlayer)
        value = appPlayer->DiscLabel();

      return true;
    }
    default:
      break;
  }

  return false;
}

bool CGamesGUIInfo::GetInt(int& value,
                           const CGUIListItem* gitem,
                           int contextWindow,
                           const CGUIInfo& info) const
{
  return false;
}

bool CGamesGUIInfo::GetBool(bool& value,
                            const CGUIListItem* gitem,
                            int contextWindow,
                            const CGUIInfo& info) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // RETROPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case RETROPLAYER_SUPPORTS_EJECT:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      value = appPlayer && appPlayer->SupportsDiscControl();

      return true;
    }
    case RETROPLAYER_DISC_EJECTED:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      value = appPlayer && appPlayer->IsDiscEjected();

      return true;
    }
    case RETROPLAYER_EMPTY_TRAY:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      value = appPlayer && appPlayer->IsTrayEmpty();

      return true;
    }
    default:
      break;
  }

  return false;
}

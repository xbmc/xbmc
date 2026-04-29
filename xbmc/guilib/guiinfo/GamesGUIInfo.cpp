/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GamesGUIInfo.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "games/addons/GameClient.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace KODI::GUILIB::GUIINFO;
using namespace KODI::GAME;
using namespace KODI::RETRO;

namespace
{
/*!
 * \brief Helper to get the currently-playing game tag
 *
 * This bypasses the global application player by using GUIInfoManager instead.
 *
 * The currently-playing item flow:
 *
 *   - Arrives via play command
 *   - Sent to the application player, which copies the item for itself
 *   - Sent to the GUIInfoManager via GUI_MSG_PLAYBACK_STARTED from the player
 *     calling CApplicationPlayerCallback::OnPlayBackStarted(). Also copies any
 *     updates back to the player.
 *   - The item can be updated at runtime via TMSG_UPDATE_PLAYER_ITEM, which
 *     updates the application's item silently and GUIInfoManager directly
 */
const CGameInfoTag* GetGUIGameTag()
{
  // Use const access because infotags can accidentally be created
  // when querying a tag that isn't set, which can completely break
  // all downstream is-video/audio/game checks
  if (const auto* gui = CServiceBroker::GetGUIConst(); gui != nullptr)
    return gui->GetInfoManager().GetCurrentGameTag();

  return nullptr;
}
} // namespace

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
      // No title in tag, derive one from the item path
      std::string title = CUtil::GetTitleFromPath(item->GetPath(), item->IsFolder());
      if (!title.empty())
        tag->SetTitle(title);
    }

    if (tag->GetPlatform().empty())
    {
      // No platform in tag, get the platform from its game client if possible
      if (const std::string& gameClient = tag->GetGameClient(); !gameClient.empty())
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(gameClient, addon, ADDON::AddonType::GAMEDLL,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
        {
          const std::string& platforms =
              std::static_pointer_cast<CGameClient>(addon)->GetPlatforms();
          if (!platforms.empty())
            tag->SetPlatform(platforms);
        }
      }
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
    case RETROPLAYER_TITLE:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetTitle();
        return true;
      }
      break;
    }
    case RETROPLAYER_PLATFORM:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetPlatform();
        return true;
      }
      break;
    }
    case RETROPLAYER_GENRES:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = StringUtils::Join(tag->GetGenres(), ", ");
        return true;
      }
      break;
    }
    case RETROPLAYER_PUBLISHER:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetPublisher();
        return true;
      }
      break;
    }
    case RETROPLAYER_DEVELOPER:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetDeveloper();
        return true;
      }
      break;
    }
    case RETROPLAYER_OVERVIEW:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetOverview();
        return true;
      }
      break;
    }
    case RETROPLAYER_GAME_CLIENT:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        value = tag->GetGameClient();
        return true;
      }
      break;
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
  return false;
}

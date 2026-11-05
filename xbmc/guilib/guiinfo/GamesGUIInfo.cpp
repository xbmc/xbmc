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
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "cores/RetroPlayer/RetroPlayerUtils.h"
#include "games/AchievementRuntime.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheats/GameClientCheats.h"
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

const CAchievementRuntime& CGamesGUIInfo::AchievementRuntime() const
{
  if (m_achievementRuntime)
    return *m_achievementRuntime;

  return CServiceBroker::GetGameServices().AchievementRuntime();
}

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
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_TITLE:
    case LISTITEM_YEAR:
    case LISTITEM_GENRE:
    case LISTITEM_PLOT:
    case LISTITEM_PLATFORM:
    case LISTITEM_DEVELOPER:
    case LISTITEM_PUBLISHER:
    case LISTITEM_ORIGINALTITLE:
    case LISTITEM_RATING:
    case LISTITEM_PLAYERS:
    case LISTITEM_AGERATING:
    case LISTITEM_TRAILER:
    case LISTITEM_TAG:
    {
      if (item == nullptr || !item->HasGameInfoTag())
        break;

      const CGameInfoTag* tag = item->GetGameInfoTag();

      switch (info.GetInfo())
      {
        case LISTITEM_TITLE:
          value = tag->GetTitle();
          break;
        case LISTITEM_YEAR:
          if (tag->GetYear() > 0)
            value = std::to_string(tag->GetYear());
          break;
        case LISTITEM_GENRE:
          value =
              StringUtils::Join(tag->GetGenres(), info.GetData3().empty() ? ", " : info.GetData3());
          break;
        case LISTITEM_PLOT:
          value = tag->GetOverview();
          break;
        case LISTITEM_PLATFORM:
          value = tag->GetPlatform();
          break;
        case LISTITEM_DEVELOPER:
          value = tag->GetDeveloper();
          break;
        case LISTITEM_PUBLISHER:
          value = tag->GetPublisher();
          break;
        case LISTITEM_ORIGINALTITLE:
          value = tag->GetOriginalTitle();
          break;
        case LISTITEM_RATING:
          if (tag->GetRating() > 0.0f)
            value = StringUtils::FormatNumber(tag->GetRating());
          break;
        case LISTITEM_PLAYERS:
          if (tag->GetPlayers() > 0)
            value = std::to_string(tag->GetPlayers());
          break;
        case LISTITEM_AGERATING:
          value = tag->GetAgeRating();
          break;
        case LISTITEM_TRAILER:
          value = tag->GetTrailer();
          break;
        case LISTITEM_TAG:
          value = StringUtils::Join(tag->GetTags(), ", ");
          break;
        default:
          break;
      }
      return !value.empty();
    }
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
    case RETROPLAYER_GAME_CLIENT_NAME:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(tag->GetGameClient(), addon,
                                                   ADDON::AddonType::GAMEDLL,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
        {
          value = std::static_pointer_cast<CGameClient>(addon)->GetEmulatorName();
          return true;
        }
      }
      break;
    }
    case RETROPLAYER_GAME_CLIENT_PLATFORMS:
    {
      if (const auto* tag = GetGUIGameTag(); tag != nullptr)
      {
        ADDON::AddonPtr addon;
        if (CServiceBroker::GetAddonMgr().GetAddon(tag->GetGameClient(), addon,
                                                   ADDON::AddonType::GAMEDLL,
                                                   ADDON::OnlyEnabled::CHOICE_YES))
        {
          value = std::static_pointer_cast<CGameClient>(addon)->GetPlatforms();
          return true;
        }
      }
      break;
    }
    case RETROPLAYER_DISC_LABEL:
    {
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();

      if (appPlayer)
        value = appPlayer->DiscLabel();

      return true;
    }
    case RETROPLAYER_RICH_PRESENCE:
    {
      value = AchievementRuntime().GetRichPresence();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_PROGRESS:
    {
      const CAchievementRuntime& runtime = AchievementRuntime();
      const unsigned int total = runtime.GetTotalAchievements();

      // An empty label lets skins hide the control instead of showing "0 / 0"
      if (total == 0)
        value.clear();
      else
        value = StringUtils::Format("{} / {}", runtime.GetUnlockedAchievements(), total);

      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE:
    case RETROPLAYER_ACHIEVEMENTS_CHALLENGE_BADGE:
    {
      // Answered empty rather than never recorded, so that turning it off while
      // an attempt is live takes the indicator off screen at once
      if (!CServiceBroker::GetGameServices().GameSettings().GetChallengeIndicator())
      {
        value.clear();
        return true;
      }

      // Counted from 1 by the skin; without a number, the first
      const unsigned int index = info.GetData1() > 0 ? info.GetData1() - 1 : 0;
      const AchievementChallenge challenge = AchievementRuntime().GetShownChallenge(index);

      value = (info.GetInfo() == RETROPLAYER_ACHIEVEMENTS_CHALLENGE_TITLE) ? challenge.title
                                                                           : challenge.badgeUrl;
      return true;
    }
    case RETROPLAYER_LEADERBOARD_TRACKER:
    {
      value = AchievementRuntime().GetShownLeaderboardTracker().display;
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE:
    case RETROPLAYER_ACHIEVEMENTS_INDICATOR_BADGE:
    case RETROPLAYER_ACHIEVEMENTS_INDICATOR_PROGRESS:
    case RETROPLAYER_ACHIEVEMENTS_INDICATOR_PERCENT:
    {
      const AchievementProgressIndicator indicator = AchievementRuntime().GetProgressIndicator();

      // The title being empty is what keeps the whole indicator hidden, so
      // every field of an inactive one must come back empty rather than "0"
      if (indicator.id == 0)
      {
        value.clear();
        return true;
      }

      switch (info.GetInfo())
      {
        case RETROPLAYER_ACHIEVEMENTS_INDICATOR_TITLE:
          value = indicator.title;
          break;
        case RETROPLAYER_ACHIEVEMENTS_INDICATOR_BADGE:
          value = indicator.badgeUrl;
          break;
        case RETROPLAYER_ACHIEVEMENTS_INDICATOR_PROGRESS:
          value = indicator.measuredProgress;
          break;
        default:
          value = std::to_string(static_cast<int>(indicator.measuredPercent));
          break;
      }
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
    case RETROPLAYER_ACHIEVEMENTS_LOGGED_IN:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementsLoggedIn();
      return true;
    }
    case RETROPLAYER_SUPPORTS_CHEATS:
    {
      const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
      value = gameClient && gameClient->Cheats().SupportsCheats();
      return true;
    }
    case RETROPLAYER_HAS_CHEATS:
    {
      const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
      value = gameClient && gameClient->Cheats().HasCheats();
      return true;
    }
    case RETROPLAYER_ACHIEVEMENTS_HARDCORE:
    {
      value = CServiceBroker::GetGameServices().GameSettings().GetAchievementsHardcore();
      return true;
    }
    default:
      break;
  }

  return false;
}

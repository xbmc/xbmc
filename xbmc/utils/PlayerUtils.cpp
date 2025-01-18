/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerUtils.h"

#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicUtils.h"
#include "playlists/PlayListFileItemClassify.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"
#include "video/guilib/VideoGUIUtils.h"

using namespace KODI;

bool CPlayerUtils::IsItemPlayable(const CFileItem& itemIn)
{
  const CFileItem item(itemIn.GetItemToPlay());

  // General
  if (item.IsParentFolder())
    return false;

  // Plugins
  if (item.IsPlugin() && item.GetProperty("isplayable").asBoolean())
    return true;

  // Music
  if (MUSIC_UTILS::IsItemPlayable(item))
    return true;

  // Movies / TV Shows / Music Videos
  if (VIDEO::UTILS::IsItemPlayable(item))
    return true;

  //! @todo add more types on demand.

  return false;
}

void CPlayerUtils::AdvanceTempoStep(const std::shared_ptr<CApplicationPlayer>& appPlayer,
                                    TempoStepChange change)
{
  const auto step = 0.1f;
  const auto currentTempo = appPlayer->GetPlayTempo();
  switch (change)
  {
    case TempoStepChange::INCREASE:
      appPlayer->SetTempo(currentTempo + step);
      break;
    case TempoStepChange::DECREASE:
      appPlayer->SetTempo(currentTempo - step);
      break;
  }
}

std::vector<std::string> CPlayerUtils::GetPlayersForItem(const CFileItem& item)
{
  const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};

  std::vector<std::string> players;
  if (VIDEO::IsVideoDb(item))
  {
    //! @todo CPlayerCoreFactory and classes called from there do not handle dyn path correctly.
    CFileItem item2{item};
    item2.SetPath(item.GetDynPath());
    playerCoreFactory.GetPlayers(item2, players);
  }
  else
  {
    playerCoreFactory.GetPlayers(item, players);
  }

  return players;
}

bool CPlayerUtils::HasItemMultiplePlayers(const CFileItem& item)
{
  return GetPlayersForItem(item).size() > 1;
}

bool CPlayerUtils::PlayMedia(const std::shared_ptr<CFileItem>& item,
                             const std::string& player,
                             KODI::PLAYLIST::Id playlistId)
{
  //! @todo get rid of special treatment for some media

  if ((MUSIC::IsAudio(*item) || VIDEO::IsVideo(*item)) && !PLAYLIST::IsSmartPlayList(*item) &&
      !item->IsPVR())
  {
    if (!item->HasProperty("playlist_type_hint"))
      item->SetProperty("playlist_type_hint", static_cast<int>(playlistId));

    PLAYLIST::CPlayListPlayer& playlistPlayer{CServiceBroker::GetPlaylistPlayer()};
    playlistPlayer.Reset();
    playlistPlayer.SetCurrentPlaylist(playlistId);
    playlistPlayer.Play(item, player);
  }
  else
  {
    g_application.PlayMedia(*item, player, playlistId);
  }
  return true;
}

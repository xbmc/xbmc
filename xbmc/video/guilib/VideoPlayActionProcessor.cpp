/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayActionProcessor.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "playlists/PlayListTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/PlayerUtils.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoVersionHelper.h"

namespace KODI::VIDEO::GUILIB
{

Action CVideoPlayActionProcessor::GetDefaultAction()
{
  return static_cast<Action>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_PLAYACTION));
}

bool CVideoPlayActionProcessor::ProcessDefaultAction()
{
  return ProcessAction(GetDefaultAction());
}

bool CVideoPlayActionProcessor::ProcessAction(Action action)
{
  m_userCancelled = false;

  const auto movie{CVideoVersionHelper::ChooseVideoFromAssets(m_item)};
  if (movie)
    m_item = movie;
  else
  {
    m_userCancelled = true;
    return true; // User cancelled the select menu. We're done.
  }

  return Process(action);
}

bool CVideoPlayActionProcessor::Process(Action action)
{
  switch (action)
  {
    case ACTION_PLAY_OR_RESUME:
    {
      const Action selectedAction = ChoosePlayOrResume(*m_item);
      if (selectedAction < 0)
      {
        m_userCancelled = true;
        return true; // User cancelled the select menu. We're done.
      }

      return Process(selectedAction);
    }

    case ACTION_RESUME:
      m_item->SetStartOffset(STARTOFFSET_RESUME);
      return OnResumeSelected();

    case ACTION_PLAY_FROM_BEGINNING:
      m_item->SetStartOffset(0);
      return OnPlaySelected();

    default:
      break;
  }
  return false; // We did not handle the action.
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume(const CFileItem& item)
{
  Action action = ACTION_PLAY_FROM_BEGINNING;

  const std::string resumeString = VIDEO::UTILS::GetResumeString(item);
  if (!resumeString.empty())
  {
    CContextButtons choices;

    choices.Add(ACTION_RESUME, resumeString);
    choices.Add(ACTION_PLAY_FROM_BEGINNING, 12021); // Play from beginning

    action = static_cast<Action>(CGUIDialogContextMenu::ShowAndGetChoice(choices));
  }

  return action;
}

bool CVideoPlayActionProcessor::OnResumeSelected()
{
  Play("");
  return true;
}

bool CVideoPlayActionProcessor::OnPlaySelected()
{
  std::string player;
  if (m_choosePlayer)
  {
    const std::vector<std::string> players{CPlayerUtils::GetPlayersForItem(*m_item)};
    const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
    player = playerCoreFactory.SelectPlayerDialog(players);
    if (player.empty())
    {
      m_userCancelled = true;
      return true; // User cancelled player selection. We're done.
    }
  }

  Play(player);
  return true;
}

void CVideoPlayActionProcessor::Play(const std::string& player)
{
  auto item{m_item};
  if (item->m_bIsFolder && item->HasVideoVersions())
  {
    //! @todo get rid of "videos with versions as folder" hack!
    item = std::make_shared<CFileItem>(*item);
    item->m_bIsFolder = false;
  }

  item->SetProperty("playlist_type_hint", static_cast<int>(KODI::PLAYLIST::Id::TYPE_VIDEO));
  const ContentUtils::PlayMode mode{item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                        ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                        : ContentUtils::PlayMode::PLAY_ONLY_THIS};
  VIDEO::UTILS::PlayItem(item, player, mode);
}

} // namespace KODI::VIDEO::GUILIB

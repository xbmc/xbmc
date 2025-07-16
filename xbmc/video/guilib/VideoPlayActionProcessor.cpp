/*
 *  Copyright (C) 2023-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayActionProcessor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "playlists/PlayListTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/PlayerUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoGUIUtils.h"

namespace KODI::VIDEO::GUILIB
{
namespace
{
Action GetDefaultPlayAction()
{
  return static_cast<Action>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_PLAYACTION));
}
} // unnamed namespace

Action CVideoPlayActionProcessor::GetDefaultAction()
{
  return GetDefaultPlayAction();
}

bool CVideoPlayActionProcessor::Process(Action action)
{
  if (m_chooseStackPart && m_chosenStackPart == 0)
  {
    if (!URIUtils::IsStack(GetItem()->GetDynPath()))
    {
      CLog::LogF(LOGERROR, "Invalid item (not a stack)!");
      return true; // done
    }

    m_chosenStackPart = ChooseStackPart();
    if (m_chosenStackPart < 1)
    {
      SetUserCancelled(true);
      return true; // User cancelled the select menu. We're done.
    }
  }

  switch (action)
  {
    case ACTION_PLAY_OR_RESUME:
    {
      const Action selectedAction = ChoosePlayOrResume();
      if (selectedAction < 0)
      {
        SetUserCancelled(true);
        return true; // User cancelled the select menu. We're done.
      }

      return Process(selectedAction);
    }

    case ACTION_RESUME:
    {
      SetResumeData();
      return OnResumeSelected();
    }

    case ACTION_PLAY_FROM_BEGINNING:
    {
      SetStartData();
      return OnPlaySelected();
    }

    default:
      break;
  }
  return false; // We did not handle the action.
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume() const
{
  if (m_chosenStackPart)
  {
    const int64_t offset{VIDEO::UTILS::GetStackPartResumeOffset(*GetItem(), m_chosenStackPart)};
    if (offset > 0)
      return ChoosePlayOrResume(VIDEO::UTILS::GetResumeString(offset, m_chosenStackPart));
  }
  else if (URIUtils::IsStack(GetItem()->GetDynPath()))
  {
    const auto [offset, partNumber] = VIDEO::UTILS::GetStackResumeOffsetAndPartNumber(*GetItem());
    if (offset > 0)
      return ChoosePlayOrResume(VIDEO::UTILS::GetResumeString(offset, partNumber));
  }
  else
  {
    const VIDEO::UTILS::ResumeInformation resumeInfo{
        VIDEO::UTILS::GetItemResumeInformation(*GetItem())};
    if (resumeInfo.isResumable)
      return ChoosePlayOrResume(
          VIDEO::UTILS::GetResumeString(resumeInfo.startOffset, resumeInfo.partNumber));
  }
  return ACTION_PLAY_FROM_BEGINNING;
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume(const std::string& resumeString)
{
  Action action = ACTION_PLAY_FROM_BEGINNING;
  if (!resumeString.empty())
  {
    CContextButtons choices;

    choices.Add(ACTION_RESUME, resumeString);
    choices.Add(ACTION_PLAY_FROM_BEGINNING, 12021); // Play from beginning

    action = static_cast<Action>(CGUIDialogContextMenu::ShowAndGetChoice(choices));
  }
  return action;
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume(const CFileItem& item)
{
  const Action action{GetDefaultPlayAction()};
  if (action == VIDEO::GUILIB::ACTION_PLAY_OR_RESUME)
    return ChoosePlayOrResume(VIDEO::UTILS::GetResumeString(item));
  else
    return action;
}

unsigned int CVideoPlayActionProcessor::ChooseStackPart() const
{
  CFileItemList parts;
  XFILE::CDirectory::GetDirectory(GetItem()->GetDynPath(), parts, "", XFILE::DIR_FLAG_DEFAULTS);

  if (parts.IsEmpty())
  {
    CLog::LogF(LOGERROR, "Invalid item (empty stack)!");
    return 0; // done
  }

  for (int i = 0; i < parts.Size(); ++i)
  {
    parts[i]->SetLabel(StringUtils::Format(g_localizeStrings.Get(23051), i + 1)); // Part #
  }

  CGUIDialogSelect* dialog{CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT)};

  dialog->Reset();
  dialog->SetHeading(CVariant{20324}); // Play part...
  dialog->SetItems(parts);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return 0; // User cancelled the dialog.

  return dialog->GetSelectedItem() + 1; // part numbers are 1-based
}

void CVideoPlayActionProcessor::SetResumeData()
{
  const auto item{GetItem()};
  if (m_chosenStackPart)
  {
    item->SetStartPartNumber(m_chosenStackPart);
    item->SetStartOffset(VIDEO::UTILS::GetStackPartResumeOffset(*item, m_chosenStackPart));
  }
  else
  {
    item->SetStartPartNumber(1);
    item->SetStartOffset(STARTOFFSET_RESUME);
  }
}

void CVideoPlayActionProcessor::SetStartData()
{
  const auto item{GetItem()};
  if (m_chosenStackPart)
  {
    item->SetStartPartNumber(m_chosenStackPart);
    item->SetStartOffset(VIDEO::UTILS::GetStackPartStartOffset(*item, m_chosenStackPart));
  }
  else
  {
    item->SetStartPartNumber(1);
    item->SetStartOffset(0);
  }
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
    const std::vector<std::string> players{CPlayerUtils::GetPlayersForItem(*GetItem())};
    const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
    player = playerCoreFactory.SelectPlayerDialog(players);
    if (player.empty())
    {
      SetUserCancelled(true);
      return true; // User cancelled player selection. We're done.
    }
  }

  Play(player);
  return true;
}

void CVideoPlayActionProcessor::Play(const std::string& player)
{
  auto item{GetItem()};
  if (item->IsFolder() && item->HasVideoVersions())
  {
    //! @todo get rid of "videos with versions as folder" hack!
    item = std::make_shared<CFileItem>(*item);
    item->SetFolder(false);
  }

  item->SetProperty("playlist_type_hint", static_cast<int>(KODI::PLAYLIST::Id::TYPE_VIDEO));
  const ContentUtils::PlayMode mode{item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                        ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                        : ContentUtils::PlayMode::PLAY_ONLY_THIS};
  VIDEO::UTILS::PlayItem(item, player, mode);
}

} // namespace KODI::VIDEO::GUILIB

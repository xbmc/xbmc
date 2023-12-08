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
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Variant.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoActionProcessorHelper.h"

using namespace VIDEO::GUILIB;

Action CVideoPlayActionProcessorBase::GetDefaultAction()
{
  return static_cast<Action>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_PLAYACTION));
}

bool CVideoPlayActionProcessorBase::ProcessDefaultAction()
{
  return ProcessAction(GetDefaultAction());
}

bool CVideoPlayActionProcessorBase::ProcessAction(Action action)
{
  m_userCancelled = false;

  CVideoActionProcessorHelper procHelper{m_item, m_videoVersion};
  const auto videoVersion{procHelper.ChooseVideoVersion()};
  if (videoVersion)
    m_item = videoVersion;
  else
  {
    m_userCancelled = true;
    return true; // User cancelled the select menu. We're done.
  }

  return Process(action);
}

bool CVideoPlayActionProcessorBase::Process(Action action)
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
      return OnResumeSelected();

    case ACTION_PLAY_FROM_BEGINNING:
      return OnPlaySelected();

    default:
      break;
  }
  return false; // We did not handle the action.
}

Action CVideoPlayActionProcessorBase::ChoosePlayOrResume(const CFileItem& item)
{
  Action action = ACTION_PLAY_FROM_BEGINNING;

  const std::string resumeString = VIDEO_UTILS::GetResumeString(item);
  if (!resumeString.empty())
  {
    CContextButtons choices;

    choices.Add(ACTION_RESUME, resumeString);
    choices.Add(ACTION_PLAY_FROM_BEGINNING, 12021); // Play from beginning

    action = static_cast<Action>(CGUIDialogContextMenu::ShowAndGetChoice(choices));
  }

  return action;
}

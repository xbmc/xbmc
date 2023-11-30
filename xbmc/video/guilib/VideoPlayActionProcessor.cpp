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

PlayAction CVideoPlayActionProcessorBase::GetDefaultPlayAction()
{
  return static_cast<PlayAction>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_PLAYACTION));
}

bool CVideoPlayActionProcessorBase::Process()
{
  return Process(GetDefaultPlayAction());
}

bool CVideoPlayActionProcessorBase::Process(PlayAction playAction)
{
  m_userCancelled = false;

  CVideoActionProcessorHelper procHelper{m_item, m_videoVersion};

  if (!m_versionChecked)
  {
    m_versionChecked = true;
    const auto videoVersion{procHelper.ChooseVideoVersion()};
    if (videoVersion)
      m_item = videoVersion;
    else
    {
      m_userCancelled = true;
      return true; // User cancelled the select menu. We're done.
    }
  }

  switch (playAction)
  {
    case PLAY_ACTION_PLAY_OR_RESUME:
    {
      const VIDEO::GUILIB::PlayAction action = ChoosePlayOrResume();
      if (action < 0)
      {
        m_userCancelled = true;
        return true; // User cancelled the select menu. We're done.
      }

      return Process(action);
    }

    case PLAY_ACTION_RESUME:
      return OnResumeSelected();

    case PLAY_ACTION_PLAY_FROM_BEGINNING:
      return OnPlaySelected();

    default:
      break;
  }
  return false; // We did not handle the action.
}

PlayAction CVideoPlayActionProcessorBase::ChoosePlayOrResume()
{
  PlayAction action = PLAY_ACTION_PLAY_FROM_BEGINNING;

  const std::string resumeString = VIDEO_UTILS::GetResumeString(*m_item);
  if (!resumeString.empty())
  {
    CContextButtons choices;

    choices.Add(PLAY_ACTION_RESUME, resumeString);
    choices.Add(PLAY_ACTION_PLAY_FROM_BEGINNING, 12021); // Play from beginning

    action = static_cast<PlayAction>(CGUIDialogContextMenu::ShowAndGetChoice(choices));
  }

  return action;
}

/*
 *  Copyright (C) 2023-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSelectActionProcessor.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Variant.h"
#include "utils/guilib/GUIContentUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoPlayActionProcessor.h"

namespace KODI::VIDEO::GUILIB
{

Action CVideoSelectActionProcessor::GetDefaultAction()
{
  auto settings{CServiceBroker::GetSettingsComponent()->GetSettings()};
  Action action{static_cast<Action>(settings->GetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION))};
  if (action == ACTION_PLAY_OR_RESUME || action == ACTION_RESUME)
  {
    // Compat: The former settings value ACTION_PLAY_OR_RESUME and ACTION_RESUME are no longer
    // valid for SETTING_MYVIDEOS_SELECTACTION. Migrate it to the new default setting value.
    action = ACTION_PLAY;
    settings->SetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION, action);
    settings->Save();
  }
  return action;
}

bool CVideoSelectActionProcessor::Process(Action action)
{
  switch (action)
  {
    case ACTION_PLAY:
      return OnPlaySelected();

    case ACTION_CHOOSE:
      return OnChooseSelected();

    case ACTION_QUEUE:
      return OnQueueSelected();

    case ACTION_INFO:
    {
      const auto item{GetItem()};
      if (GetDefaultAction() == ACTION_INFO && !KODI::VIDEO::IsVideoDb(*item) &&
          !item->IsPlugin() && !item->IsScript() && !item->IsPVR() &&
          !KODI::VIDEO::UTILS::HasItemVideoDbInformation(*item))
      {
        // For items without info fall back to default play action.
        return Process(ACTION_PLAY);
      }

      return OnInfoSelected();
    }

    default:
      break;
  }
  return false; // We did not handle the action.
}

bool CVideoSelectActionProcessor::OnPlaySelected()
{
  // Execute default play action.
  CVideoPlayActionProcessor proc(GetItem());
  return proc.ProcessDefaultAction();
}

bool CVideoSelectActionProcessor::OnQueueSelected()
{
  VIDEO::UTILS::QueueItem(GetItem(), VIDEO::UTILS::QueuePosition::POSITION_END);
  return true;
}

bool CVideoSelectActionProcessor::OnInfoSelected()
{
  return KODI::UTILS::GUILIB::CGUIContentUtils::ShowInfoForItem(*GetItem());
}

bool CVideoSelectActionProcessor::OnChooseSelected()
{
  CONTEXTMENU::ShowFor(GetItem(), CContextMenuManager::MAIN);
  return true;
}

} // namespace KODI::VIDEO::GUILIB

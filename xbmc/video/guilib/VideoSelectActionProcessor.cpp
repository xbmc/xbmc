/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSelectActionProcessor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "video/VideoFileItemClassify.h"
#include "video/guilib/VideoGUIUtils.h"

namespace KODI::VIDEO::GUILIB
{

Action CVideoSelectActionProcessorBase::GetDefaultSelectAction()
{
  return static_cast<Action>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_SELECTACTION));
}

Action CVideoSelectActionProcessorBase::GetDefaultAction()
{
  return GetDefaultSelectAction();
}

bool CVideoSelectActionProcessorBase::Process(Action action)
{
  if (CVideoPlayActionProcessorBase::Process(action))
    return true;

  switch (action)
  {
    case ACTION_CHOOSE:
      return OnChooseSelected();

    case ACTION_PLAYPART:
    {
      const unsigned int part = ChooseStackItemPartNumber();
      if (part < 1) // part numbers are 1-based
        return false;

      return OnPlayPartSelected(part);
    }

    case ACTION_QUEUE:
      return OnQueueSelected();

    case ACTION_INFO:
    {
      if (GetDefaultAction() == ACTION_INFO && !KODI::VIDEO::IsVideoDb(*m_item) &&
          !m_item->IsPlugin() && !m_item->IsScript() &&
          !KODI::VIDEO::UTILS::HasItemVideoDbInformation(*m_item))
      {
        // for items without info fall back to default play action
        return Process(CVideoPlayActionProcessorBase::GetDefaultAction());
      }

      return OnInfoSelected();
    }

    default:
      break;
  }
  return false; // We did not handle the action.
}

unsigned int CVideoSelectActionProcessorBase::ChooseStackItemPartNumber() const
{
  CFileItemList parts;
  XFILE::CDirectory::GetDirectory(m_item->GetDynPath(), parts, "", XFILE::DIR_FLAG_DEFAULTS);

  for (int i = 0; i < parts.Size(); ++i)
    parts[i]->SetLabel(StringUtils::Format(g_localizeStrings.Get(23051), i + 1)); // Part #

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);

  dialog->Reset();
  dialog->SetHeading(CVariant{20324}); // Play part...
  dialog->SetItems(parts);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return 0; // User cancelled the dialog.

  return dialog->GetSelectedItem() + 1; // part numbers are 1-based
}

} // namespace KODI::VIDEO::GUILIB

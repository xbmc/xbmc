/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAudioSelect.h"

#include "Application.h"
#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include "system.h"

using namespace ADDON;
using namespace XFILE;

#define STRING_AUDIO_HEADER 460
#define STRING_TOAST_TEXT_ERROR 416

CGUIDialogAudioSelect::CGUIDialogAudioSelect(void) : CGUIDialog(WINDOW_DIALOG_AUDIO_SELECT, "")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogAudioSelect::~CGUIDialogAudioSelect(void)
{
}

bool CGUIDialogAudioSelect::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogAudioSelect::OnInitWindow()
{
  ShowAudioSelect();

  CGUIWindow::OnInitWindow();
}

bool CGUIDialogAudioSelect::ShowAudioSelect()
{
  //Only display when playing
  if (!g_application.GetAppPlayer().HasPlayer())
  {
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info, g_localizeStrings.Get(STRING_AUDIO_HEADER),
        g_localizeStrings.Get(STRING_TOAST_TEXT_ERROR), TOAST_DISPLAY_TIME, false);
    return false;
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (dialog == NULL)
    return false;

  CFileItemList options;

  int audioStreamCount = g_application.GetAppPlayer().GetAudioStreamCount();
  int currentAudio = g_application.GetAppPlayer().GetAudioStream();

  // cycle through each subtitle and add it to our entry list
  for (int i = 0; i < audioStreamCount; ++i)
  {
    std::string strItem;
    std::string strLanguage;

    AudioStreamInfo info;
    g_application.GetAppPlayer().GetAudioStreamInfo(i, info);

    if (!g_LangCodeExpander.Lookup(info.language, strLanguage))
      strLanguage = g_localizeStrings.Get(13205); // Unknown

    if (info.name.length() == 0)
      strItem = strLanguage;
    else
      strItem = StringUtils::Format("%s - %s", strLanguage.c_str(), info.name.c_str());

    CFileItemPtr item(new CFileItem(strItem.c_str()));
    item->SetProperty("value", i);

    if (i == currentAudio)
      item->Select(true);

    options.Add(item);
  }

  if (options.Size() < 2)
  {
    CGUIDialogKaiToast::QueueNotification(
        CGUIDialogKaiToast::Info, g_localizeStrings.Get(STRING_AUDIO_HEADER),
        g_localizeStrings.Get(STRING_TOAST_TEXT_ERROR), TOAST_DISPLAY_TIME, false);
    return true;
  }

  dialog->Reset();
  dialog->SetHeading(g_localizeStrings.Get(STRING_AUDIO_HEADER).c_str());
  dialog->SetItems(options);
  dialog->SetMultiSelection(false);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return true;

  int selected = dialog->GetSelectedItem();
  if (selected != currentAudio)
  {
    g_application.GetAppPlayer().SetAudioStream(selected);
  }

  return true;
}

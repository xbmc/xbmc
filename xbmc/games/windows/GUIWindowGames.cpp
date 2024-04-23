/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowGames.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "application/Application.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "media/MediaLockState.h"
#include "playlists/PlayListTypes.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY 3
#define CONTROL_BTNSORTASC 4

CGUIWindowGames::CGUIWindowGames() : CGUIMediaWindow(WINDOW_GAMES, "MyGames.xml")
{
}

bool CGUIWindowGames::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(true); //! @todo

      // Is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CMediaSourceSettings::GetInstance().GetDefaultSource("games"));

      //! @todo
      m_dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);

      break;
    }
    case GUI_MSG_CLICKED:
    {
      if (OnClickMsg(message.GetSenderId(), message.GetParam1()))
        return true;
      break;
    }
    default:
      break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowGames::OnClickMsg(int controlId, int actionId)
{
  if (!m_viewControl.HasControl(controlId)) // list/thumb control
    return false;

  const int iItem = m_viewControl.GetSelectedItem();

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (!pItem)
    return false;

  switch (actionId)
  {
    case ACTION_DELETE_ITEM:
    {
      // Is delete allowed?
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_FILELISTS_ALLOWFILEDELETION))
      {
        OnDeleteItem(iItem);
        return true;
      }
      break;
    }
    case ACTION_PLAYER_PLAY:
    {
      if (OnClick(iItem))
        return true;
      break;
    }
    case ACTION_SHOW_INFO:
    {
      if (!m_vecItems->IsPlugin())
      {
        if (pItem->HasAddonInfo())
        {
          CGUIDialogAddonInfo::ShowForItem(pItem);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }

  return false;
}

void CGUIWindowGames::SetupShares()
{
  CGUIMediaWindow::SetupShares();

  // Don't convert zip files to directories. Otherwise, the files will be
  // opened and scanned for games with a valid extension. If none are found,
  // the .zip won't be shown.
  //
  // This is a problem for MAME roms, because the files inside the .zip don't
  // have standard extensions.
  //
  m_rootDir.SetFlags(XFILE::DIR_FLAG_NO_FILE_DIRS);
}

bool CGUIWindowGames::OnClick(int iItem, const std::string& player /* = "" */)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item)
  {
    // Compensate for DIR_FLAG_NO_FILE_DIRS flag
    if (URIUtils::IsArchive(item->GetPath()))
    {
      bool bIsGame = false;

      // If zip file contains no games, assume it is a game
      CFileItemList items;
      if (m_rootDir.GetDirectory(CURL(item->GetPath()), items))
      {
        if (items.Size() == 0)
          bIsGame = true;
      }

      if (!bIsGame)
        item->m_bIsFolder = true;
    }

    if (!item->m_bIsFolder)
    {
      PlayGame(*item);
      return true;
    }
  }

  return CGUIMediaWindow::OnClick(iItem, player);
}

void CGUIWindowGames::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
  {
    if (m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->IsSourcesPath())
    {
      // Context buttons for a sources path, like "Add Source", "Remove Source", etc.
      CGUIDialogContextMenu::GetContextButtons("games", item, buttons);
    }
    else
    {
      if (item->IsGame())
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
      }

      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) &&
          !item->IsReadOnly())
      {
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      }
    }
  }

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowGames::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item)
  {
    if (m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->IsSourcesPath())
    {
      if (CGUIDialogContextMenu::OnContextButton("games", item, button))
      {
        Update(m_vecItems->GetPath());
        return true;
      }
    }
    switch (button)
    {
      case CONTEXT_BUTTON_PLAY_ITEM:
        PlayGame(*item);
        return true;
      case CONTEXT_BUTTON_INFO:
        CGUIDialogAddonInfo::ShowForItem(item);
        return true;
      case CONTEXT_BUTTON_DELETE:
        OnDeleteItem(itemNumber);
        return true;
      case CONTEXT_BUTTON_RENAME:
        OnRenameItem(itemNumber);
        return true;
      default:
        break;
    }
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowGames::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("games");
}

bool CGUIWindowGames::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  // Set label
  std::string label;
  if (items.GetLabel().empty())
  {
    std::string source;
    if (m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("games"),
                           &source))
      label = std::move(source);
  }

  if (!label.empty())
    items.SetLabel(label);

  // Set content
  std::string content;
  if (items.GetContent().empty())
  {
    if (!items.IsVirtualDirectoryRoot() && // Don't set content for root directory
        !items.IsPlugin()) // Don't set content for plugins
    {
      content = "games";
    }
  }

  if (!content.empty())
    items.SetContent(content);

  // Ensure a game info tag is created so that files are recognized as games
  for (const CFileItemPtr& item : items)
  {
    if (!item->m_bIsFolder)
      item->GetGameInfoTag();
  }

  return true;
}

std::string CGUIWindowGames::GetStartFolder(const std::string& dir)
{
  // From CGUIWindowPictures::GetStartFolder()

  if (StringUtils::EqualsNoCase(dir, "plugins") || StringUtils::EqualsNoCase(dir, "addons"))
  {
    return "addons://sources/game/";
  }

  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex >= 0)
  {
    if (iIndex < static_cast<int>(shares.size()) && shares[iIndex].m_iHasLock == LOCK_STATE_LOCKED)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item, "games"))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowGames::OnItemInfo(int itemNumber)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (!m_vecItems->IsPlugin())
  {
    if (item->IsPlugin() || item->IsScript())
      CGUIDialogAddonInfo::ShowForItem(item);
  }

  //! @todo
  /*
  CGUIDialogGameInfo* gameInfo =
  CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogGameInfo>(WINDOW_DIALOG_PICTURE_INFO);
  if (gameInfo)
  {
    gameInfo->SetGame(item);
    gameInfo->Open();
  }
  */
}

bool CGUIWindowGames::PlayGame(const CFileItem& item)
{
  CFileItem itemCopy(item);
  return g_application.PlayMedia(itemCopy, "", PLAYLIST::TYPE_NONE);
}

/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowGames.h"
#include "addons/GUIDialogAddonInfo.h"
#include "Application.h"
#include "dialogs/GUIDialogProgress.h"
#include "FileItem.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "guilib/WindowIDs.h"
#include "GUIPassword.h"
#include "settings/Settings.h"
#include "URL.h"
#include "Util.h"

#define CONTROL_BTNVIEWASICONS      2
#define CONTROL_BTNSORTBY           3
#define CONTROL_BTNSORTASC          4
//#define CONTROL_LABELFILES         12

using namespace XFILE;

CGUIWindowGames::CGUIWindowGames() : CGUIMediaWindow(WINDOW_GAMES, "MyGames.xml")
{
}

bool CGUIWindowGames::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(false);

      // Is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        m_vecItems->SetPath("");

      m_dlgProgress = dynamic_cast<CGUIDialogProgress*>(g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS));
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        CFileItemPtr pItem = m_vecItems->Get(iItem);
        int iAction = message.GetParam1();

        if (iAction == ACTION_DELETE_ITEM)
        {
          if (CSettings::Get().GetBool("filelists.allowfiledeletion"))
          {
            OnDeleteItem(iItem);
            return true;
          }
        }
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          return OnClick(iItem);
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          if (!m_vecItems->IsPlugin() && pItem && (pItem->IsPlugin() || pItem->IsScript()))
          {
            CGUIDialogAddonInfo::ShowForItem(pItem);
            return true;
          }
        }
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowGames::SetupShares()
{
  CGUIMediaWindow::SetupShares();
  // XBMC downloads a list of supported extensions from the remote add-ons
  // repo. Zip files are treated as directories and scanned recursively; if
  // they don't contain valid extensions (such as MAME arcade games), the
  // entire zip will be missing from the MyGames window. Skipping the recursive
  // scan always shows zip files (note: entering the zip will show an empty
  // folder) and speeds up directory listing as a nice side effect.
  m_rootDir.SetFlags(DIR_FLAG_NO_FILE_DIRS);
}

void CGUIWindowGames::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
  {
    if (m_vecItems->IsSourcesPath())
    {
      // Context buttons for a sources path, like "Add source", "Remove Source", etc.
      CGUIDialogContextMenu::GetContextButtons("games", item, buttons);
    }
    else if (!m_vecItems->IsVirtualDirectoryRoot())
    {
      if (item->IsGame())
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
        // Let RetroPlayer handle this one
        //buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213); // Play With...
      }

      if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
        buttons.Add(CONTEXT_BUTTON_INFO, 24003); // Add-on info

      if (CSettings::Get().GetBool("filelists.allowfiledeletion") && !item->IsReadOnly())
      {
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      }

      if (item->IsPlugin() || item->IsScript() || m_vecItems->IsPlugin())
      {
        buttons.Add(CONTEXT_BUTTON_PLUGIN_SETTINGS, 1045);
      }
    }
    else
    {
      // We are virtual directory root
      buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523); // Switch media
    }
  }

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
    buttons.Add(CONTEXT_BUTTON_SETTINGS, 5); // Settings
}

bool CGUIWindowGames::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item && m_vecItems->IsSourcesPath())
  {
    if (CGUIDialogContextMenu::OnContextButton("games", item, button))
    {
      Update("sources://games/");
      return true;
    }
  }
  switch (button)
  {
  case CONTEXT_BUTTON_PLAY_ITEM:
  case CONTEXT_BUTTON_PLAY_WITH:
    return item && PlayGame(*item);
  case CONTEXT_BUTTON_INFO:
    OnInfo(itemNumber);
    return true;
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_SETTINGS:
    g_windowManager.ActivateWindow(WINDOW_SETTINGS_MYGAMES);
    return true;
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowGames::OnClick(int itemNumber)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return true;

  if (!(item->m_bIsFolder || item->IsFileFolder()) && item->IsGame())
    return PlayGame(*item);
  else
    return CGUIMediaWindow::OnClick(itemNumber);
}

void CGUIWindowGames::OnInfo(int itemNumber)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
    CGUIDialogAddonInfo::ShowForItem(item);
}

bool CGUIWindowGames::PlayGame(const CFileItem &item)
{
  CFileItem gameFile = item;
  CURL url(item.GetPath());
  if (url.GetProtocol() == "zip" && url.GetFileName() == "")
    gameFile = CFileItem(url.GetHostName(), false);

  // Allocate a game info tag to let the player know it's a game
  gameFile.GetGameInfoTag();

  // Let RetroPlayer choose the right action henceforth
  return g_application.PlayFile(gameFile) == PLAYBACK_OK;
}

CStdString CGUIWindowGames::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("Plugins") || dir.Equals("Addons"))
    return "addons://sources/game/";

  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex >= 0)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
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

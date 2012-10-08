/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "GUIWindowSettingsProfile.h"
#include "windows/GUIWindowFileManager.h"
#include "Profile.h"
#include "Application.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"
#include "network/Network.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "GUIPassword.h"
#include "windows/GUIWindowLoginScreen.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "Settings.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE;

#define CONTROL_PROFILES 2
#define CONTROL_LASTLOADED_PROFILE 3
#define CONTROL_LOGINSCREEN 4

CGUIWindowSettingsProfile::CGUIWindowSettingsProfile(void)
    : CGUIWindow(WINDOW_SETTINGS_PROFILES, "SettingsProfile.xml")
{
  m_listItems = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowSettingsProfile::~CGUIWindowSettingsProfile(void)
{
  delete m_listItems;
}

int CGUIWindowSettingsProfile::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES);
  g_windowManager.SendMessage(msg);

  return msg.GetParam1();
}

void CGUIWindowSettingsProfile::OnPopupMenu(int iItem)
{
  if (iItem == (int)g_settings.GetNumProfiles())
    return;

  // popup the context menu
  CContextButtons choices;
  choices.Add(1, 20092); // Load profile
  if (iItem > 0)
    choices.Add(2, 117); // Delete

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (choice == 1)
  {
    unsigned iCtrlID = GetFocusedControlID();
    g_application.StopPlaying();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, g_windowManager.GetActiveWindow(), iCtrlID);
    g_windowManager.SendMessage(msg2);
    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_settings.LoadMasterForLogin();
    CGUIWindowLoginScreen::LoadProfile(iItem);
    return;
  }

  if (choice == 2)
  {
    if (g_settings.DeleteProfile(iItem))
      iItem--;
  }

  LoadList();
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(),CONTROL_PROFILES,iItem);
  OnMessage(msg);
}

bool CGUIWindowSettingsProfile::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      ClearListItems();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PROFILES)
      {
        int iAction = message.GetParam1();
        if (
          iAction == ACTION_SELECT_ITEM ||
          iAction == ACTION_MOUSE_LEFT_CLICK ||
          iAction == ACTION_CONTEXT_MENU ||
          iAction == ACTION_MOUSE_RIGHT_CLICK
        )
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
          {
            //contextmenu
            if (iItem <= (int)g_settings.GetNumProfiles() - 1)
            {
              OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem < (int)g_settings.GetNumProfiles())
          {
            if (CGUIDialogProfileSettings::ShowForProfile(iItem))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              g_windowManager.SendMessage(msg);

              return true;
            }

            return false;
          }
          else if (iItem > (int)g_settings.GetNumProfiles() - 1)
          {
            CDirectory::Create(URIUtils::AddFileToFolder(g_settings.GetUserDataFolder(),"profiles"));
            if (CGUIDialogProfileSettings::ShowForProfile(g_settings.GetNumProfiles()))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              g_windowManager.SendMessage(msg);
              return true;
            }

            return false;
          }
        }
      }
      else if (iControl == CONTROL_LOGINSCREEN)
      {
        g_settings.ToggleLoginScreen();
        g_settings.SaveProfiles(PROFILES_FILE);
        return true;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSettingsProfile::LoadList()
{
  ClearListItems();

  for (unsigned int i = 0; i < g_settings.GetNumProfiles(); i++)
  {
    const CProfile *profile = g_settings.GetProfile(i);
    CFileItemPtr item(new CFileItem(profile->getName()));
    item->SetLabel2(profile->getDate());
    item->SetArt("thumb", profile->getThumb());
    item->SetOverlayImage(profile->getLockMode() == LOCK_MODE_EVERYONE ? CGUIListItem::ICON_OVERLAY_NONE : CGUIListItem::ICON_OVERLAY_LOCKED);
    m_listItems->Add(item);
  }
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(20058)));
    m_listItems->Add(item);
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_PROFILES, 0, 0, m_listItems);
  OnMessage(msg);

  if (g_settings.UsingLoginScreen())
  {
    CONTROL_SELECT(CONTROL_LOGINSCREEN);
  }
  else
  {
    CONTROL_DESELECT(CONTROL_LOGINSCREEN);
  }
}

void CGUIWindowSettingsProfile::ClearListItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PROFILES);
  g_windowManager.SendMessage(msg);

  m_listItems->Clear();
}

void CGUIWindowSettingsProfile::OnInitWindow()
{
  LoadList();
  CGUIWindow::OnInitWindow();
}


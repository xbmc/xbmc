/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIWindowFileManager.h"
#include "Profile.h"
#include "Util.h"
#include "Application.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"
#include "utils/Network.h"
#include "utils/GUIInfoManager.h"
#include "utils/Weather.h"
#include "GUIPassword.h"

using namespace DIRECTORY;

#define CONTROL_PROFILES 2
#define CONTROL_LASTLOADED_PROFILE 3
#define CONTROL_LOGINSCREEN 4

CGUIWindowSettingsProfile::CGUIWindowSettingsProfile(void)
    : CGUIWindow(WINDOW_SETTINGS_PROFILES, "SettingsProfile.xml")
{
}

CGUIWindowSettingsProfile::~CGUIWindowSettingsProfile(void)
{}

bool CGUIWindowSettingsProfile::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

int CGUIWindowSettingsProfile::GetSelectedItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  return msg.GetParam1();
}

void CGUIWindowSettingsProfile::OnPopupMenu(int iItem)
{
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_PROFILES);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return ;
  // load our menu
  pMenu->Initialize();
  if (iItem == (int)g_settings.m_vecProfiles.size())
    return;

  // add the needed buttons
  int btnLoad = pMenu->AddButton(20092); // load profile
  int btnDelete=0;
  if (iItem > 0)
    btnDelete = pMenu->AddButton(117); // Delete

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();
  int iButton = pMenu->GetButton();
  if (iButton == btnLoad)
  {
    unsigned iCtrlID = GetFocusedControlID();
    g_application.StopPlaying();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, m_gWindowManager.GetActiveWindow(), iCtrlID, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
    g_network.NetworkMessage(CNetwork::SERVICES_DOWN,1);
#ifdef HAS_XBOX_NETWORK
    g_network.Deinitialize();
#endif
    bool bOldMaster = g_passwordManager.bMasterUser;
    g_passwordManager.bMasterUser = true;
    g_settings.LoadProfile(iItem);

    g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate();
    g_settings.SaveProfiles("q:\\system\\profiles.xml"); // to set last loaded

    g_passwordManager.bMasterUser = bOldMaster;
#ifdef HAS_XBOX_NETWORK
    g_network.Initialize(g_guiSettings.GetInt("network.assignment"),
      g_guiSettings.GetString("network.ipaddress").c_str(),
      g_guiSettings.GetString("network.subnet").c_str(),
      g_guiSettings.GetString("network.gateway").c_str(),
      g_guiSettings.GetString("network.dns").c_str());
#endif
    CGUIMessage msg3(GUI_MSG_SETFOCUS, m_gWindowManager.GetActiveWindow(), iCtrlID, 0);
    OnMessage(msg3);
    CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, m_gWindowManager.GetActiveWindow(), iCtrlID, msg2.GetParam1(), msg2.GetParam2());
    OnMessage(msgSelect);
    g_weatherManager.Refresh();
  }

  if (iButton == btnDelete)
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
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES, 0, 0, NULL);
          g_graphicsContext.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
          {
            //contextmenu
            if (iItem <= (int)g_settings.m_vecProfiles.size() - 1)
            {
              OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem < (int)g_settings.m_vecProfiles.size())
          {
            if (CGUIDialogProfileSettings::ShowForProfile(iItem))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              m_gWindowManager.SendMessage(msg);

              return true;
            }

            return false;
          }
          else if (iItem > (int)g_settings.m_vecProfiles.size() - 1)
          {
            CDirectory::Create(g_settings.GetUserDataFolder()+"\\profiles");
            if (CGUIDialogProfileSettings::ShowForProfile(g_settings.m_vecProfiles.size()))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              m_gWindowManager.SendMessage(msg);
              return true;
            }

            return false;
          }
        }
      }
      else if (iControl == CONTROL_LOGINSCREEN)
      {
        g_settings.bUseLoginScreen = !g_settings.bUseLoginScreen;
        g_settings.SaveProfiles("q:\\system\\profiles.xml");
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

  for (UCHAR i = 0; i < g_settings.m_vecProfiles.size(); i++)
  {
    CProfile& profile = g_settings.m_vecProfiles.at(i);
    CFileItem* item = new CFileItem(profile.getName());
    item->m_strPath.Empty();
    item->SetLabel2(profile.getDate());
    item->SetThumbnailImage(profile.getThumb());
    item->SetOverlayImage(profile.getLockMode() == LOCK_MODE_EVERYONE ? CGUIListItem::ICON_OVERLAY_NONE : CGUIListItem::ICON_OVERLAY_LOCKED);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PROFILES, 0, 0, (void*)item);
    g_graphicsContext.SendMessage(msg);
    m_vecListItems.push_back(item);
  }
  {
    CFileItem* item = new CFileItem(g_localizeStrings.Get(20058));
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_PROFILES, 0, 0, (void*)item);
    g_graphicsContext.SendMessage(msg);
    item->m_strPath.Empty();
    m_vecListItems.push_back(item);
  }

  if (g_settings.bUseLoginScreen)
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
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PROFILES, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  for (int i = 0;i < (int)m_vecListItems.size();++i)
  {
    CGUIListItem* pListItem = m_vecListItems[i];
    delete pListItem;
  }

  m_vecListItems.erase(m_vecListItems.begin(), m_vecListItems.end());
}

void CGUIWindowSettingsProfile::OnInitWindow()
{
  LoadList();
  CGUIWindow::OnInitWindow();
}


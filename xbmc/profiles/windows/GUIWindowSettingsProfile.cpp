/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "profiles/Profile.h"
#include "profiles/ProfilesManager.h"
#include "Application.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogSelect.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "network/Network.h"
#include "utils/URIUtils.h"
#include "GUIPassword.h"
#include "windows/GUIWindowLoginScreen.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"

using namespace XFILE;

#define CONTROL_PROFILES 2
#define CONTROL_LASTLOADED_PROFILE 3
#define CONTROL_LOGINSCREEN 4
#define CONTROL_AUTOLOGIN 5

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
  if (iItem == (int)CProfilesManager::Get().GetNumberOfProfiles())
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
    CProfilesManager::Get().LoadMasterProfileForLogin();
    CGUIWindowLoginScreen::LoadProfile(iItem);
    return;
  }

  if (choice == 2)
  {
    if (CProfilesManager::Get().DeleteProfile(iItem))
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
            if (iItem <= (int)CProfilesManager::Get().GetNumberOfProfiles() - 1)
            {
              OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem < (int)CProfilesManager::Get().GetNumberOfProfiles())
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
          else if (iItem > (int)CProfilesManager::Get().GetNumberOfProfiles() - 1)
          {
            CDirectory::Create(URIUtils::AddFileToFolder(CProfilesManager::Get().GetUserDataFolder(),"profiles"));
            if (CGUIDialogProfileSettings::ShowForProfile(CProfilesManager::Get().GetNumberOfProfiles()))
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
        CProfilesManager::Get().ToggleLoginScreen();
        CProfilesManager::Get().Save();
        return true;
      }
      else if (iControl == CONTROL_AUTOLOGIN)
      {
        int currentId = CProfilesManager::Get().GetAutoLoginProfileId();
        int profileId;
        if (GetAutoLoginProfileChoice(profileId) && (currentId != profileId))
        {
          CProfilesManager::Get().SetAutoLoginProfileId(profileId);
          CProfilesManager::Get().Save();
        }
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

  for (unsigned int i = 0; i < CProfilesManager::Get().GetNumberOfProfiles(); i++)
  {
    const CProfile *profile = CProfilesManager::Get().GetProfile(i);
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

  if (CProfilesManager::Get().UsingLoginScreen())
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

bool CGUIWindowSettingsProfile::GetAutoLoginProfileChoice(int &iProfile)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog) return false;

  // add items
  // "Last used profile" option comes first, so up indices by 1
  int autoLoginProfileId = CProfilesManager::Get().GetAutoLoginProfileId() + 1;
  CFileItemList items;
  CFileItemPtr item(new CFileItem());
  item->SetLabel(g_localizeStrings.Get(37014)); // Last used profile
  item->SetIconImage("unknown-user.png");
  items.Add(item);

  for (unsigned int i = 0; i < CProfilesManager::Get().GetNumberOfProfiles(); i++)
  {
    const CProfile *profile = CProfilesManager::Get().GetProfile(i);
    std::string locked = g_localizeStrings.Get(profile->getLockMode() > 0 ? 20166 : 20165);
    CFileItemPtr item(new CFileItem(profile->getName()));
    item->SetProperty("Addon.Summary", locked); // lock setting
    std::string thumb = profile->getThumb();
    if (thumb.empty())
      thumb = "unknown-user.png";
    item->SetIconImage(thumb);
    items.Add(item);
  }

  dialog->SetHeading(20093); // Profile name
  dialog->Reset();
  dialog->SetItems(&items);
  dialog->SetSelected(autoLoginProfileId);
  dialog->DoModal();

  if (dialog->IsButtonPressed() || dialog->GetSelectedLabel() < 0)
    return false; // user cancelled
  iProfile = dialog->GetSelectedLabel() - 1;

  return true;
}

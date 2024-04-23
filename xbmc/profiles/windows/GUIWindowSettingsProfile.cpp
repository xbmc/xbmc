/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSettingsProfile.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "profiles/Profile.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "windows/GUIWindowFileManager.h"

using namespace XFILE;

#define CONTROL_PROFILES 2
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
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return msg.GetParam1();
}

void CGUIWindowSettingsProfile::OnPopupMenu(int iItem)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (iItem == (int)profileManager->GetNumberOfProfiles())
    return;

  // popup the context menu
  CContextButtons choices;
  choices.Add(1, 20092); // Load profile
  if (iItem > 0)
    choices.Add(2, 117); // Delete

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (choice == 1)
  {
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_LOADPROFILE, iItem);
    return;
  }

  if (choice == 2)
  {
    if (profileManager->DeleteProfile(iItem))
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
          const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROFILES);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
          {
            //contextmenu
            if (iItem <= static_cast<int>(profileManager->GetNumberOfProfiles()) - 1)
            {
              OnPopupMenu(iItem);
            }
            return true;
          }
          else if (iItem < static_cast<int>(profileManager->GetNumberOfProfiles()))
          {
            if (CGUIDialogProfileSettings::ShowForProfile(iItem))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

              return true;
            }

            return false;
          }
          else if (iItem > static_cast<int>(profileManager->GetNumberOfProfiles()) - 1)
          {
            CDirectory::Create(URIUtils::AddFileToFolder(profileManager->GetUserDataFolder(),"profiles"));
            if (CGUIDialogProfileSettings::ShowForProfile(profileManager->GetNumberOfProfiles()))
            {
              LoadList();
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), 2,iItem);
              CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
              return true;
            }

            return false;
          }
        }
      }
      else if (iControl == CONTROL_LOGINSCREEN)
      {
        const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

        profileManager->ToggleLoginScreen();
        profileManager->Save();
        return true;
      }
      else if (iControl == CONTROL_AUTOLOGIN)
      {
        const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

        int currentId = profileManager->GetAutoLoginProfileId();
        int profileId;
        if (GetAutoLoginProfileChoice(profileId) && (currentId != profileId))
        {
          profileManager->SetAutoLoginProfileId(profileId);
          profileManager->Save();
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

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  for (unsigned int i = 0; i < profileManager->GetNumberOfProfiles(); i++)
  {
    const CProfile *profile = profileManager->GetProfile(i);
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

  if (profileManager->UsingLoginScreen())
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
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  m_listItems->Clear();
}

void CGUIWindowSettingsProfile::OnInitWindow()
{
  LoadList();
  CGUIWindow::OnInitWindow();
}

bool CGUIWindowSettingsProfile::GetAutoLoginProfileChoice(int &iProfile)
{
  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!dialog) return false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // add items
  // "Last used profile" option comes first, so up indices by 1
  int autoLoginProfileId = profileManager->GetAutoLoginProfileId() + 1;
  CFileItemList items;
  CFileItemPtr item(new CFileItem());
  item->SetLabel(g_localizeStrings.Get(37014)); // Last used profile
  item->SetArt("icon", "DefaultUser.png");
  items.Add(item);

  for (unsigned int i = 0; i < profileManager->GetNumberOfProfiles(); i++)
  {
    const CProfile *profile = profileManager->GetProfile(i);
    const std::string& locked = g_localizeStrings.Get(profile->getLockMode() > 0 ? 20166 : 20165);
    CFileItemPtr item(new CFileItem(profile->getName()));
    item->SetLabel2(locked); // lock setting
    std::string thumb = profile->getThumb();
    if (thumb.empty())
      thumb = "DefaultUser.png";
    item->SetArt("icon", thumb);
    items.Add(item);
  }

  dialog->SetHeading(CVariant{20093}); // Profile name
  dialog->Reset();
  dialog->SetUseDetails(true);
  dialog->SetItems(items);
  dialog->SetSelected(autoLoginProfileId);
  dialog->Open();

  if (dialog->IsButtonPressed() || dialog->GetSelectedItem() < 0)
    return false; // user cancelled
  iProfile = dialog->GetSelectedItem() - 1;

  return true;
}

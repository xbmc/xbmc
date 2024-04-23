/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowLoginScreen.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "profiles/Profile.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/ViewState.h"

using namespace KODI::MESSAGING;

#define CONTROL_BIG_LIST               52
#define CONTROL_LABEL_HEADER            2
#define CONTROL_LABEL_SELECTED_PROFILE  3

CGUIWindowLoginScreen::CGUIWindowLoginScreen(void)
  : CGUIWindow(WINDOW_LOGIN_SCREEN, "LoginScreen.xml")
{
  watch.StartZero();
  m_vecItems = new CFileItemList;
  m_iSelectedItem = -1;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowLoginScreen::~CGUIWindowLoginScreen(void)
{
  delete m_vecItems;
}

bool CGUIWindowLoginScreen::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_vecItems->Clear();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BIG_LIST)
      {
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          int iItem = m_viewControl.GetSelectedItem();
          bool bResult = OnPopupMenu(m_viewControl.GetSelectedItem());
          if (bResult)
          {
            Update();
            CGUIMessage msg(GUI_MSG_ITEM_SELECT,GetID(),CONTROL_BIG_LIST,iItem);
            OnMessage(msg);
          }

          return bResult;
        }
        else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          int iItem = m_viewControl.GetSelectedItem();
          bool bCanceled;
          bool bOkay = g_passwordManager.IsProfileLockUnlocked(iItem, bCanceled);

          if (bOkay)
          {
            if (iItem >= 0)
              CServiceBroker::GetAppMessenger()->PostMsg(TMSG_LOADPROFILE, iItem);
          }
          else
          {
            if (!bCanceled && iItem != 0)
              HELPERS::ShowOKDialogText(CVariant{20068}, CVariant{20117});
          }
        }
      }
    }
    break;
    case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    default:
    break;

  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowLoginScreen::OnAction(const CAction &action)
{
  // don't allow built in actions to act here except shutdown related ones.
  // this forces only navigation type actions to be performed.
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    std::string actionName = action.GetName();
    StringUtils::ToLower(actionName);
    if ((actionName.find("shutdown") != std::string::npos) &&
        CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
      CBuiltins::GetInstance().Execute(action.GetName());
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowLoginScreen::OnBack(int actionID)
{
  // no escape from the login window
  return false;
}

void CGUIWindowLoginScreen::FrameMove()
{
  if (GetFocusedControlID() == CONTROL_BIG_LIST && !CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true))
  {
    if (m_viewControl.HasControl(CONTROL_BIG_LIST))
      m_iSelectedItem = m_viewControl.GetSelectedItem();
  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20114), m_iSelectedItem + 1,
                                             profileManager->GetNumberOfProfiles());
  SET_CONTROL_LABEL(CONTROL_LABEL_SELECTED_PROFILE,strLabel);
  CGUIWindow::FrameMove();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  m_iSelectedItem = static_cast<int>(profileManager->GetLastUsedProfileIndex());

  // Update list/thumb control
  m_viewControl.SetCurrentView(DEFAULT_VIEW_LIST);
  Update();
  m_viewControl.SetFocused();
  SET_CONTROL_LABEL(CONTROL_LABEL_HEADER,g_localizeStrings.Get(20115));
  SET_CONTROL_VISIBLE(CONTROL_BIG_LIST);

  CGUIWindow::OnInitWindow();
}

void CGUIWindowLoginScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_BIG_LIST));
}

void CGUIWindowLoginScreen::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIWindowLoginScreen::Update()
{
  m_vecItems->Clear();

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  for (unsigned int i = 0; i < profileManager->GetNumberOfProfiles(); ++i)
  {
    const CProfile *profile = profileManager->GetProfile(i);

    CFileItemPtr item(new CFileItem(profile->getName()));

    std::string strLabel;
    if (profile->getDate().empty())
      strLabel = g_localizeStrings.Get(20113);
    else
      strLabel = StringUtils::Format(g_localizeStrings.Get(20112), profile->getDate());

    item->SetLabel2(strLabel);
    item->SetArt("thumb", profile->getThumb());
    if (profile->getThumb().empty())
      item->SetArt("thumb", "DefaultUser.png");
    item->SetLabelPreformatted(true);

    m_vecItems->Add(item);
  }
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(m_iSelectedItem);
}

bool CGUIWindowLoginScreen::OnPopupMenu(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  bool bSelect = pItem->IsSelected();

  // mark the item
  pItem->Select(true);

  CContextButtons choices;
  choices.Add(1, 20067);

  if (iItem == 0 && g_passwordManager.iMasterLockRetriesLeft == 0)
    choices.Add(2, 12334);

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (choice == 2)
  {
    if (g_passwordManager.CheckLock(profileManager->GetMasterProfile().getLockMode(), profileManager->GetMasterProfile().getLockCode(), 20075))
      g_passwordManager.iMasterLockRetriesLeft = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES);
    else // be inconvenient
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SHUTDOWN);

    return true;
  }

  // Edit the profile after checking if the correct master lock password was given.
  if (choice == 1 && g_passwordManager.IsMasterLockUnlocked(true))
    CGUIDialogProfileSettings::ShowForProfile(m_viewControl.GetSelectedItem());

  //NOTE: this can potentially (de)select the wrong item if the filelisting has changed because of an action above.
  if (iItem < static_cast<int>(profileManager->GetNumberOfProfiles()))
    m_vecItems->Get(iItem)->Select(bSelect);

  return false;
}

CFileItemPtr CGUIWindowLoginScreen::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (item < 0 || !m_vecItems->Size()) return CFileItemPtr();

  item = (item + offset) % m_vecItems->Size();
  if (item < 0) item += m_vecItems->Size();
  return m_vecItems->Get(item);
}

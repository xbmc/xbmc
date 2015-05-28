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

#include "system.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIWindowLoginScreen.h"
#include "profiles/Profile.h"
#include "profiles/ProfilesManager.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIPassword.h"
#ifdef HAS_JSONRPC
#include "interfaces/json-rpc/JSONRPC.h"
#endif
#include "interfaces/Builtins.h"
#include "utils/log.h"
#include "utils/Weather.h"
#include "utils/StringUtils.h"
#include "network/Network.h"
#include "addons/Skin.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "addons/AddonManager.h"
#include "view/ViewState.h"
#include "pvr/PVRManager.h"
#include "ContextMenuManager.h"

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
              LoadProfile((unsigned int)iItem);
          }
          else
          {
            if (!bCanceled && iItem != 0)
              CGUIDialogOK::ShowAndGetInput(20068, 20117);
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
        PVR::g_PVRManager.CanSystemPowerdown())
      CBuiltins::Execute(action.GetName());
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
  if (GetFocusedControlID() == CONTROL_BIG_LIST && g_windowManager.GetTopMostModalDialogID() == WINDOW_INVALID)
    if (m_viewControl.HasControl(CONTROL_BIG_LIST))
      m_iSelectedItem = m_viewControl.GetSelectedItem();
  std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20114).c_str(), m_iSelectedItem+1, CProfilesManager::Get().GetNumberOfProfiles());
  SET_CONTROL_LABEL(CONTROL_LABEL_SELECTED_PROFILE,strLabel);
  CGUIWindow::FrameMove();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
  m_iSelectedItem = (int)CProfilesManager::Get().GetLastUsedProfileIndex();
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
  for (unsigned int i=0;i<CProfilesManager::Get().GetNumberOfProfiles(); ++i)
  {
    const CProfile *profile = CProfilesManager::Get().GetProfile(i);
    CFileItemPtr item(new CFileItem(profile->getName()));
    std::string strLabel;
    if (profile->getDate().empty())
      strLabel = g_localizeStrings.Get(20113);
    else
      strLabel = StringUtils::Format(g_localizeStrings.Get(20112).c_str(), profile->getDate().c_str());
    item->SetLabel2(strLabel);
    item->SetArt("thumb", profile->getThumb());
    if (profile->getThumb().empty() || profile->getThumb() == "-")
      item->SetArt("thumb", "unknown-user.png");
    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(m_iSelectedItem);
}

bool CGUIWindowLoginScreen::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() ) return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  bool bSelect = pItem->IsSelected();
  // mark the item
  pItem->Select(true);

  CContextButtons choices;
  choices.Add(1, 20067);

  if (iItem == 0 && g_passwordManager.iMasterLockRetriesLeft == 0)
    choices.Add(2, 12334);

  CContextMenuManager::Get().AddVisibleItems(pItem, choices);

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (choice == 2)
  {
    if (g_passwordManager.CheckLock(CProfilesManager::Get().GetMasterProfile().getLockMode(),CProfilesManager::Get().GetMasterProfile().getLockCode(),20075))
      g_passwordManager.iMasterLockRetriesLeft = CSettings::Get().GetInt("masterlock.maxretries");
    else // be inconvenient
      CApplicationMessenger::Get().Shutdown();

    return true;
  }

  // Edit the profile after checking if the correct master lock password was given.
  if (choice == 1 && g_passwordManager.IsMasterLockUnlocked(true))
    CGUIDialogProfileSettings::ShowForProfile(m_viewControl.GetSelectedItem());
  
  //NOTE: this can potentially (de)select the wrong item if the filelisting has changed because of an action above.
  if (iItem < (int)CProfilesManager::Get().GetNumberOfProfiles())
    m_vecItems->Get(iItem)->Select(bSelect);

  if (choice >= CONTEXT_BUTTON_FIRST_ADDON)
    return CContextMenuManager::Get().Execute(choice, pItem);
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

void CGUIWindowLoginScreen::LoadProfile(unsigned int profile)
{
  // stop service addons and give it some time before we start it again
  ADDON::CAddonMgr::Get().StopServices(true);

  // stop PVR related services
  g_application.StopPVRManager();

  if (profile != 0 || !CProfilesManager::Get().IsMasterProfile())
  {
    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    CProfilesManager::Get().LoadProfile(profile);
  }
  else
  {
    CGUIWindow* pWindow = g_windowManager.GetWindow(WINDOW_HOME);
    if (pWindow)
      pWindow->ResetControlStates();
  }
  g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_UP,1);

  CProfilesManager::Get().UpdateCurrentProfileDate();
  CProfilesManager::Get().Save();

  if (CProfilesManager::Get().GetLastUsedProfileIndex() != profile)
  {
    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }

  // reload the add-ons, or we will first load all add-ons from the master account without checking disabled status
  ADDON::CAddonMgr::Get().ReInit();

  if (!g_application.LoadLanguage(true))
  {
    CLog::Log(LOGFATAL, "CGUIWindowLoginScreen: unable to load language for profile \"%s\"", CProfilesManager::Get().GetCurrentProfile().getName().c_str());
    return;
  }

  g_weatherManager.Refresh();
  g_application.SetLoggingIn(true);

#ifdef HAS_JSONRPC
  JSONRPC::CJSONRPC::Initialize();
#endif

  // start services which should run on login 
  ADDON::CAddonMgr::Get().StartServices(false);

  // start PVR related services
  g_application.StartPVRManager();

  int firstWindow = g_SkinInfo->GetFirstWindow();
  // the startup window is considered part of the initialization as it most likely switches to the final window
  bool uiInitializationFinished = firstWindow != WINDOW_STARTUP_ANIM;

  g_windowManager.ChangeActiveWindow(firstWindow);

  g_application.UpdateLibraries();
  CStereoscopicsManager::Get().Initialize();

  // if the user interfaces has been fully initialized let everyone know
  if (uiInitializationFinished)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UI_READY);
    g_windowManager.SendThreadMessage(msg);
  }
}

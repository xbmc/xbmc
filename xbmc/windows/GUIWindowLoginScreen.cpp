/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "Application.h"
#include "GUIWindowLoginScreen.h"
#include "settings/GUIWindowSettingsProfile.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "settings/GUIDialogProfileSettings.h"
#include "GUIPassword.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#ifdef HAS_JSONRPC
#include "interfaces/json-rpc/JSONRPC.h"
#endif
#include "interfaces/Builtins.h"
#include "utils/Weather.h"
#include "network/Network.h"
#include "addons/Skin.h"
#include "settings/Profile.h"
#include "guilib/GUIMessage.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "addons/AddonManager.h"

#define CONTROL_BIG_LIST               52
#define CONTROL_LABEL_HEADER            2
#define CONTROL_LABEL_SELECTED_PROFILE  3

CGUIWindowLoginScreen::CGUIWindowLoginScreen(void)
: CGUIWindow(WINDOW_LOGIN_SCREEN, "LoginScreen.xml")
{
  watch.StartZero();
  m_vecItems = new CFileItemList;
  m_iSelectedItem = -1;
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
      m_viewControl.Reset();
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
            LoadProfile(iItem);
          }
          else
          {
            if (!bCanceled && iItem != 0)
              CGUIDialogOK::ShowAndGetInput(20068,20117,20022,20022);
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
    CStdString actionName = action.GetName();
    actionName.ToLower();
    if (actionName.Find("shutdown") != -1)
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
  CStdString strLabel;
  strLabel.Format(g_localizeStrings.Get(20114),m_iSelectedItem+1,g_settings.GetNumProfiles());
  SET_CONTROL_LABEL(CONTROL_LABEL_SELECTED_PROFILE,strLabel);
  CGUIWindow::FrameMove();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
  m_iSelectedItem = (int)g_settings.GetLastUsedProfileIndex();
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

void CGUIWindowLoginScreen::Update()
{
  m_vecItems->Clear();
  for (unsigned int i=0;i<g_settings.GetNumProfiles(); ++i)
  {
    const CProfile *profile = g_settings.GetProfile(i);
    CFileItemPtr item(new CFileItem(profile->getName()));
    CStdString strLabel;
    if (profile->getDate().IsEmpty())
      strLabel = g_localizeStrings.Get(20113);
    else
      strLabel.Format(g_localizeStrings.Get(20112), profile->getDate());
    item->SetLabel2(strLabel);
    item->SetThumbnailImage(profile->getThumb());
    if (profile->getThumb().IsEmpty() || profile->getThumb().Equals("-"))
      item->SetThumbnailImage("unknown-user.png");
    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(m_iSelectedItem);
}

bool CGUIWindowLoginScreen::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() ) return false;

  bool bSelect = m_vecItems->Get(iItem)->IsSelected();
  // mark the item
  m_vecItems->Get(iItem)->Select(true);

  CContextButtons choices;
  choices.Add(1, 20067);
/*  if (m_viewControl.GetSelectedItem() != 0) // no deleting the default profile
    choices.Add(2, 117); */
  if (iItem == 0 && g_passwordManager.iMasterLockRetriesLeft == 0)
    choices.Add(3, 12334);

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
  if (choice == 3)
  {
    if (g_passwordManager.CheckLock(g_settings.GetMasterProfile().getLockMode(),g_settings.GetMasterProfile().getLockCode(),20075))
      g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
    else // be inconvenient
      g_application.getApplicationMessenger().Shutdown();

    return true;
  }
  
  if (!g_passwordManager.IsMasterLockUnlocked(true))
    return false;

  if (choice == 1)
    CGUIDialogProfileSettings::ShowForProfile(m_viewControl.GetSelectedItem());
  if (choice == 2)
  {
    int iDelete = m_viewControl.GetSelectedItem();
    m_viewControl.Clear();
    g_settings.DeleteProfile(iDelete);
    Update();
    m_viewControl.SetSelectedItem(0);
  }
  //NOTE: this can potentially (de)select the wrong item if the filelisting has changed because of an action above.
  if (iItem < (int)g_settings.GetNumProfiles())
    m_vecItems->Get(iItem)->Select(bSelect);

  return (choice > 0);
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

  if (profile != 0 || !g_settings.IsMasterUser())
  {
    g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
    g_settings.LoadProfile(profile);
  }
  else
  {
    CGUIWindow* pWindow = g_windowManager.GetWindow(WINDOW_HOME);
    if (pWindow)
      pWindow->ResetControlStates();
  }
  g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_UP,1);

  g_settings.UpdateCurrentProfileDate();
  g_settings.SaveProfiles(PROFILES_FILE);

  if (g_settings.GetLastUsedProfileIndex() != profile)
  {
    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  }

  g_weatherManager.Refresh();
#ifdef HAS_PYTHON
  g_pythonParser.m_bLogin = true;
#endif

#ifdef HAS_JSONRPC
  JSONRPC::CJSONRPC::Initialize();
#endif

  // start services which should run on login 
  ADDON::CAddonMgr::Get().StartServices(false);

  g_windowManager.ChangeActiveWindow(g_SkinInfo->GetFirstWindow());

  g_application.UpdateLibraries();
}

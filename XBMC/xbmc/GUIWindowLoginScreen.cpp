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
#include "Application.h"
#include "GUIWindowLoginScreen.h"
#include "GUIWindowSettingsProfile.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogProfileSettings.h"
#include "utils/GUIInfoManager.h"
#include "GUIPassword.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif
#include "lib/libscrobbler/scrobbler.h"
#include "utils/Weather.h"
#include "utils/FanController.h"
#include "utils/Network.h"
#include "SkinInfo.h"

using namespace XFILE;

#define CONTROL_BIG_LIST 52
#define CONTROL_LABEL_HEADER 2
#define CONTROL_LABEL_SELECTED_PROFILE 3

CGUIWindowLoginScreen::CGUIWindowLoginScreen(void)
: CGUIWindow(WINDOW_LOGIN_SCREEN, "LoginScreen.xml")
{
  watch.StartZero();
}

CGUIWindowLoginScreen::~CGUIWindowLoginScreen(void)
{
}

bool CGUIWindowLoginScreen::OnMessage(CGUIMessage& message)
{
  bool bResult=false;
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_viewControl.Reset();
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
        else if (iAction == ACTION_PREVIOUS_MENU) // oh no u don't
          return false;
        else if (iAction == ACTION_SELECT_ITEM)
        {
          int iItem = m_viewControl.GetSelectedItem();
          bool bOkay = !g_guiSettings.GetBool("masterlock.loginlock");
          bool bCanceled;
          if (!bOkay)
            bOkay = g_passwordManager.IsProfileLockUnlocked(m_viewControl.GetSelectedItem(), bCanceled);

          if (bOkay)
          {
            if (CFile::Exists("q:\\scripts\\autoexec.py") && watch.GetElapsedMilliseconds() < 5000.f)
              while (watch.GetElapsedMilliseconds() < 5000) ;
            if (iItem != 0 || g_settings.m_iLastLoadedProfileIndex != 0)
            {
              g_application.getNetwork().NetworkMessage(CNetwork::SERVICES_DOWN,1);
#ifdef HAS_XBOX_NETWORK
              g_application.getNetwork().Deinitialize();
#endif
#ifdef HAS_XBOX_HARDWARE
              CLog::Log(LOGNOTICE, "stop fancontroller");
              CFanController::Instance()->Stop();
#endif
              g_settings.LoadProfile(m_viewControl.GetSelectedItem());
#ifdef HAS_XBOX_NETWORK
              g_application.getNetwork().Initialize(g_guiSettings.GetInt("network.assignment"),
                g_guiSettings.GetString("network.ipaddress").c_str(),
                g_guiSettings.GetString("network.subnet").c_str(),
                g_guiSettings.GetString("network.gateway").c_str(),
                g_guiSettings.GetString("network.dns").c_str());
#endif
            }
            else
            {
              CGUIWindow* pWindow = m_gWindowManager.GetWindow(WINDOW_HOME);
              if (pWindow)
                pWindow->ResetControlStates();
            }

            g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate();
            g_settings.SaveProfiles("q:\\system\\profiles.xml");

            g_weatherManager.Refresh();
#ifdef HAS_PYTHON
            g_pythonParser.bLogin = true;
#endif
            RESOLUTION res=INVALID;
            CStdString startupPath = g_SkinInfo.GetSkinPath("startup.xml", &res);
            int startWindow = g_guiSettings.GetInt("lookandfeel.startupwindow");
            // test for a startup window, and activate that instead of home
            if (CFile::Exists(startupPath) && (!g_SkinInfo.OnlyAnimateToHome() || startWindow == WINDOW_HOME))
            {
              m_gWindowManager.ChangeActiveWindow(WINDOW_STARTUP);
            }
            else
            {
              m_gWindowManager.ChangeActiveWindow(WINDOW_HOME);
              m_gWindowManager.ActivateWindow(g_guiSettings.GetInt("lookandfeel.startupwindow"));
            }

#ifdef HAS_KAI
            if (iItem == 0)
              g_application.StartKai();
#endif
            return true;
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
  // don't allow any built in actions to act here.
  // this forces only navigation type actions to be performed.
  if (action.wID == ACTION_BUILT_IN_FUNCTION)
    return true;  // pretend we handled it
  return CGUIWindow::OnAction(action);
}

void CGUIWindowLoginScreen::Render()
{
  if (GetFocusedControlID() == CONTROL_BIG_LIST && m_gWindowManager.GetTopMostModalDialogID() == WINDOW_INVALID)
    if (m_viewControl.HasControl(CONTROL_BIG_LIST))
      m_iSelectedItem = m_viewControl.GetSelectedItem();
  CStdString strLabel;
  strLabel.Format(g_localizeStrings.Get(20114),m_iSelectedItem+1,g_settings.m_vecProfiles.size());
  SET_CONTROL_LABEL(CONTROL_LABEL_SELECTED_PROFILE,strLabel);
  CGUIWindow::Render();
}

void CGUIWindowLoginScreen::OnInitWindow()
{
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
  m_vecItems.Clear();
  for (unsigned int i=0;i<g_settings.m_vecProfiles.size(); ++i)
  {
    CFileItem item(g_settings.m_vecProfiles[i].getName());
    CStdString strLabel;
    if (g_settings.m_vecProfiles[i].getDate().IsEmpty())
      strLabel = g_localizeStrings.Get(20113);
    else
      strLabel.Format(g_localizeStrings.Get(20112),g_settings.m_vecProfiles[i].getDate());
    item.SetLabel2(strLabel);
    item.SetThumbnailImage(g_settings.m_vecProfiles[i].getThumb());
    if (g_settings.m_vecProfiles[i].getThumb().IsEmpty() || g_settings.m_vecProfiles[i].getThumb().Equals("-"))
      item.SetThumbnailImage("unknown-user.png");
    item.SetLabelPreformated(true);
    m_vecItems.Add(new CFileItem(item));
  }
  m_viewControl.SetItems(m_vecItems);
  if (g_settings.m_iLastUsedProfileIndex > -1)
  {
    m_viewControl.SetSelectedItem(g_settings.m_iLastUsedProfileIndex);
    g_settings.m_iLastUsedProfileIndex = -1;
  }
}

bool CGUIWindowLoginScreen::OnPopupMenu(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size() ) return false;
  // calculate our position
  float posX = 200, posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_BIG_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }

  bool bSelect = m_vecItems[iItem]->IsSelected();
  // mark the item
  m_vecItems[iItem]->Select(true);

  // popup the context menu
  CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  if (!pMenu) return false;

  // initialize the menu (loaded on demand)
  pMenu->Initialize();

  int btn_EditProfile   = pMenu->AddButton(20067);
  int btn_DeleteProfile = 0;
  int btn_ResetLock = 0;
/*  if (m_viewControl.GetSelectedItem() != 0) // no deleting the default profile
    btn_DeleteProfile = pMenu->AddButton(117); */
  if (iItem == 0 && g_passwordManager.iMasterLockRetriesLeft == 0)
    btn_ResetLock = pMenu->AddButton(12334);

  // position it correctly
  pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
  pMenu->DoModal();

  int btnid = pMenu->GetButton();
  if (btnid > 0)
  {
    if (btnid == btn_ResetLock)
    {
      if (g_passwordManager.CheckLock(g_settings.m_vecProfiles[0].getLockMode(),g_settings.m_vecProfiles[0].getLockCode(),20075))
        g_passwordManager.iMasterLockRetriesLeft = g_guiSettings.GetInt("masterlock.maxretries");
      else // be inconvenient
        g_application.getApplicationMessenger().Shutdown();

      return true;
    }
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

    if (btnid == btn_EditProfile)
      CGUIDialogProfileSettings::ShowForProfile(m_viewControl.GetSelectedItem());
    if (btnid == btn_DeleteProfile)
    {
      int iDelete = m_viewControl.GetSelectedItem();
      m_viewControl.Clear();
      g_settings.DeleteProfile(iDelete);
      Update();
      m_viewControl.SetSelectedItem(0);
    }
  }
  //NOTE: this can potentially (de)select the wrong item if the filelisting has changed because of an action above.
  if (iItem < (int)g_settings.m_vecProfiles.size())
    m_vecItems[iItem]->Select(bSelect);

  return (btnid > 0);
}

CFileItem* CGUIWindowLoginScreen::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (item < 0 || !m_vecItems.Size()) return NULL;

  item = (item + offset) % m_vecItems.Size();
  if (item < 0) item += m_vecItems.Size();
  return m_vecItems[item];
}

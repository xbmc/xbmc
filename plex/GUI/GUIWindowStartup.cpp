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

#include "GUIWindowStartup.h"
#include "guilib/Key.h"
#include "PlexApplication.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "Application.h"
#include "GUI/GUIDialogPlexUserSelect.h"
#include "GUISettings.h"
#include "PlexTypes.h"
#include "log.h"
#include "PlexDirectory.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "input/XBMC_vkeys.h"

#define CONTROL_LIST 3

#define CONTROL_NUM0 10
#define CONTROL_NUM9 19
#define CONTROL_BACKSPACE 23

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowStartup::CGUIWindowStartup(void)
  : CGUIWindow(WINDOW_STARTUP_ANIM, "PlexUserSelect.xml"), m_allowEscOut(true)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowStartup::~CGUIWindowStartup(void) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowStartup::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    m_selectedUser = "";
    m_selectedUserThumb = "";
    m_pin = "";

    m_users.Clear();
    m_viewControl.Reset();

    g_windowManager.setRetrictedAccess(true);

    /* Add the old user, so it can work offline as well */
    if (g_plexApplication.myPlexManager)
    {
      CMyPlexUserInfo info = g_plexApplication.myPlexManager->GetCurrentUserInfo();
      if (info.id != -1)
      {
        CFileItemPtr oldUser = CFileItemPtr(new CFileItem);
        oldUser->SetLabel(info.username);
        oldUser->SetProperty("restricted", info.restricted);
        oldUser->SetProperty("protected", !info.pin.empty());
        oldUser->SetProperty("id", info.id);
        oldUser->SetArt("thumb", info.thumb);

        m_users.Add(oldUser);
        m_viewControl.SetItems(m_users);
      }
    }

    if (g_plexApplication.myPlexManager->IsSignedIn())
    {
      if (!fetchUsers())
        return false;
    }
  }

  if (message.GetMessage() == GUI_MSG_MYPLEX_STATE_CHANGE)
  {
    // users might have changed, let's refetch them.
    if (g_plexApplication.myPlexManager->IsSignedIn())
    {
      if (!fetchUsers())
      {
        CLog::Log(LOGERROR, "Unable to Fetch users, going back to home");
        PreviousWindow();
        return true;
      }
    }
  }

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // check if user was selected
    int iAction = message.GetParam1();
    if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
    {
      int iSelected = m_viewControl.GetSelectedItem();
      if (iSelected >= 0 && iSelected < (int)m_users.Size())
      {
        OnUserSelected(m_users.Get(iSelected));
      }
    }

    // check if we have some pin entry
    int iControl = message.GetSenderId();
    if (CONTROL_NUM0 <= iControl &&
        iControl <= CONTROL_NUM9) // User numeric entry via dialog button UI
    {
      OnNumber(iControl - 10);
      return true;
    }
    else if (iControl == CONTROL_BACKSPACE)
    {
      OnBackSpace();
      return true;
    }
  }

  return CGUIWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST));
  m_viewControl.SetCurrentView(CONTROL_LIST);

  // focus the user list control
  CGUIControl* list = (CGUIControl*)GetControl(CONTROL_LIST);
  if (list)
  {
    list->SetFocus(true);
  }

  // select current user
  std::string currentUsername = g_plexApplication.myPlexManager->GetCurrentUserInfo().username;
  SelectUserByName(currentUsername);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowStartup::OnAction(const CAction& action)
{
  if (action.IsMouse())
    return true;

  // pin keys input
  if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
    OnNumber(action.GetID() - REMOTE_0);
  else if (action.GetID() == ACTION_BACKSPACE)
    OnBackSpace();
  else if (action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    BYTE b = action.GetID() & 0xFF;
    if (b == XBMCVK_BACK)
      OnBackSpace();
  }
  else if (action.GetID() >= KEY_ASCII)
  {
    if (action.GetUnicode() >= 48 && action.GetUnicode() < 58) // number
      OnNumber(action.GetUnicode() - 48);
    else if (action.GetUnicode() == 8)
      OnBackSpace(); // backspace
  }
  else if (action.GetID() == ACTION_NAV_BACK)
  {
    if (m_allowEscOut)
      PreviousWindow();

    return true;
  }

  return CGUIWindow::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowStartup::fetchUsers()
{
  XFILE::CPlexDirectory dir;
  CFileItemList users;
  if (dir.GetDirectory(CURL("plexserver://myplex/api/home/users"), users))
  {
    std::string currentUsername = g_plexApplication.myPlexManager->GetCurrentUserInfo().username;

    m_users.Clear();
    m_users.Copy(users);

    for (int i = 0; i < m_users.Size(); i++)
    {
      CFileItemPtr item = m_users.Get(i);
      if (item->GetProperty("restricted").asBoolean() == false)
        item->ClearProperty("restricted");
      if (item->GetProperty("protected").asBoolean() == false)
        item->ClearProperty("protected");
      if (item->GetProperty("admin").asBoolean() == false)
        item->ClearProperty("admin");
    }

    m_viewControl.SetItems(m_users);
  }
  else if (dir.IsTokenInvalid())
  {
    CLog::Log(LOGDEBUG, "CGUIDialogPlexUserSelect::fetchUser got a invalid token!");
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::SelectUserByName(CStdString user)
{
  for (int i = 0; i < m_users.Size(); i++)
  {
    if (m_users.Get(i)->GetLabel() == user)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::OnUserSelected(CFileItemPtr item)
{
  bool isAdmin = item->GetProperty("admin").asBoolean();
  bool close = false;

  m_selectedUser = item->GetProperty("title").asString();
  m_selectedUserThumb = item->GetArt("thumb");

  if (!item->GetProperty("protected").asBoolean())
  {
    // no PIN needed.
    if (g_plexApplication.myPlexManager->GetCurrentUserInfo().id !=
        item->GetProperty("id").asInteger())
    {
      g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(-1));
    }

    m_selectedUser = "";
    m_selectedUserThumb = "";
    PreviousWindow();
  }
  else
  {
    // we selected a user that requires a pin, ask for pincode
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(52624),
                                          g_localizeStrings.Get(52626));
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::OnNumber(unsigned int num)
{
  m_pin += num + '0';

  CFileItemPtr item = m_users.Get(m_viewControl.GetSelectedItem());

  if (item && (m_pin.length() == 4))
  {
    // we got a full pin (4 chars), check it its valid
    if (g_plexApplication.myPlexManager->VerifyPin(m_pin, item->GetProperty("id").asInteger()))
    {
      if (g_plexApplication.myPlexManager->GetCurrentUserInfo().id !=
          item->GetProperty("id").asInteger())
      {
        g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(), m_pin);
      }

      PreviousWindow();
    }
    else
    {
      // we got an invalid pin
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(52624),
                                            g_localizeStrings.Get(52625));
      m_pin = "";
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::OnBackSpace()
{
  if (!m_pin.IsEmpty())
    m_pin.Delete(m_pin.GetLength() - 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::PreviousWindow()
{
  m_allowEscOut = true;
  g_windowManager.setRetrictedAccess(false);
  g_windowManager.PreviousWindow();
}

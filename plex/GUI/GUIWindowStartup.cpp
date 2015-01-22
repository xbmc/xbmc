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

#define CONTROL_LIST          3

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowStartup::CGUIWindowStartup(void)
    : CGUIWindow(WINDOW_STARTUP_ANIM, "PlexUserSelect.xml")
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowStartup::~CGUIWindowStartup(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowStartup::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    m_authed = false;
    m_userSwitched = false;
    m_selectedUser = "";
    m_selectedUserThumb = "";

    m_users.Clear();
    m_viewControl.Reset();

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

        SelectUserByName(info.username);
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
        g_windowManager.PreviousWindow();
        return true;
      }
    }
  }

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iAction = message.GetParam1();
    if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
    {
      int iSelected = m_viewControl.GetSelectedItem();
      if(iSelected >= 0 && iSelected < (int)m_users.Size())
      {
        OnUserSelected(m_users.Get(iSelected));
      }
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

  if (g_plexApplication.myPlexManager->IsPinProtected() && !g_guiSettings.GetBool("myplex.automaticlogin"))
  {
//    CGUIDialogPlexUserSelect* dialog = (CGUIDialogPlexUserSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_PLEX_USER_SELECT);
//    if (dialog)
//    {
//      while(true)
//      {
//        dialog->DoModal();
//        if (dialog->DidAuth())
//        {
//          if (dialog->DidSwitchUser())
//            g_windowManager.ActivateWindow(WINDOW_HOME);
//          else
//            g_windowManager.PreviousWindow();
//          break;
//        }
//      }
//    }
  }
  else
  {
    g_windowManager.PreviousWindow();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowStartup::OnAction(const CAction &action)
{
  if (action.IsMouse())
    return true;
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

    for (int i = 0; i < m_users.Size(); i ++)
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

    SelectUserByName(currentUsername);
  }
  else if (dir.IsTokenInvalid())
  {
    CLog::Log(LOGDEBUG, "CGUIDialogPlexUserSelect::fetchUser got a invalid token!");
    // not much more we can do at this point. We failed
    // to get our user list because we had a invalid token
    // so we just navigate "home"
    //
    m_authed = true;
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowStartup::SelectUserByName(CStdString user)
{
  for (int i=0; i < m_users.Size(); i++)
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
  return;

  bool isAdmin = item->GetProperty("admin").asBoolean();
  bool close = false;

  if (item->GetProperty("protected").asBoolean())
  {
    bool firstTry = true;
    m_selectedUser = item->GetProperty("title").asString();
    m_selectedUserThumb = item->GetArt("thumb");
    while (true)
    {
      CStdString pin;
      CGUIDialogNumeric* diag = (CGUIDialogNumeric*)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
      if (diag)
      {
        CStdString initial = "";
        diag->SetMode(CGUIDialogNumeric::INPUT_PASSWORD, (void*)&initial);
        diag->SetHeading(firstTry ? "Enter PIN" : "Try again...");
        diag->DoModal();

        if (!diag->IsConfirmed() || diag->IsCanceled())
        {
          close = false;
          break;
        }
        else
        {
          diag->GetOutput(&pin);
          if (g_plexApplication.myPlexManager->VerifyPin(pin, item->GetProperty("id").asInteger()))
          {
            m_authed = true;
            if (g_plexApplication.myPlexManager->GetCurrentUserInfo().id != item->GetProperty("id").asInteger())
            {
              m_userSwitched = true;
              g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(), pin);
            }
            break;
          }
          firstTry = false;
        }
      }
    }
  }
  else
  {
    // no PIN needed.
    m_authed = true;
    if (g_plexApplication.myPlexManager->GetCurrentUserInfo().id != item->GetProperty("id").asInteger())
    {
      m_userSwitched = true;
      g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(-1));
    }
  }

  if (close)
  {
    m_selectedUser = "";
    m_selectedUserThumb = "";
    g_windowManager.PreviousWindow();
  }
}

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

#include "stdafx.h"
#include "GUIDialogTVGroupManager.h"
#include "PVRManager.h"
#include "Application.h"
#include "Util.h"
#include "Picture.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "ViewState.h"
#include "Settings.h"
#include "FileItem.h"

using namespace std;

#define CONTROL_LIST_CHANNELS_LEFT    11
#define CONTROL_LIST_CHANNELS_RIGHT   12
#define CONTROL_LIST_CHANNEL_GROUPS   13

#define CONTROL_CURRENT_GROUP_LABEL   20

#define BUTTON_NEWGROUP               26
#define BUTTON_RENAMEGROUP            27
#define BUTTON_DELGROUP               28
#define BUTTON_OK                     29

CGUIDialogTVGroupManager::CGUIDialogTVGroupManager()
    : CGUIDialog(WINDOW_DIALOG_TV_GROUP_MANAGER, "DialogTVGroupManager.xml")
{
  m_channelLeftItems  = new CFileItemList;
  m_channelRightItems = new CFileItemList;
  m_channelGroupItems = new CFileItemList;
}

CGUIDialogTVGroupManager::~CGUIDialogTVGroupManager()
{
  delete m_channelLeftItems;
  delete m_channelRightItems;
  delete m_channelGroupItems;
}

bool CGUIDialogTVGroupManager::OnMessage(CGUIMessage& message)
{
  unsigned int iControl = 0;
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {

    case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_iSelectedLeft = 0;
      m_iSelectedRight = 0;
      m_iSelectedGroup = 0;
      Update();
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      iControl = message.GetSenderId();

      if (iControl == BUTTON_OK)
      {
        Close();
      }
      else if (iControl == BUTTON_NEWGROUP)
      {
        CStdString strDescription = "";
        if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, g_localizeStrings.Get(18130), false))
        {
          if (strDescription != "")
          {
            CPVRManager::GetInstance()->AddGroup(strDescription);
            Update();
          }
        }
      }
      else if (iControl == BUTTON_DELGROUP && m_channelGroupItems->GetFileCount() != 0)
      {
        m_iSelectedGroup      = m_viewControlGroup.GetSelectedItem();
        CFileItemPtr pItemGroup   = m_channelGroupItems->Get(m_iSelectedGroup);

        // prompt user for confirmation of channel record
        CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);

        if (pDialog)
        {
          pDialog->SetHeading(117);
          pDialog->SetLine(0, "");
          pDialog->SetLine(1, CPVRManager::GetInstance()->GetGroupName(atoi(pItemGroup->m_strPath.c_str())));
          pDialog->SetLine(2, "");
          pDialog->DoModal();

          if (pDialog->IsConfirmed())
          {
            CPVRManager::GetInstance()->DeleteGroup(atoi(pItemGroup->m_strPath.c_str()));
            Update();
          }
        }
        return true;
      }
      else if (iControl == BUTTON_RENAMEGROUP && m_channelGroupItems->GetFileCount() != 0)
      {
        if (CGUIDialogKeyboard::ShowAndGetInput(m_CurrentGroupName, g_localizeStrings.Get(18130), false))
        {
          if (m_CurrentGroupName != "")
          {
            CPVRManager::GetInstance()->RenameGroup(atoi(m_channelGroupItems->Get(m_iSelectedGroup)->m_strPath.c_str()), m_CurrentGroupName);
          }
          Update();
        }
      }
      else if (m_viewControlLeft.HasControl(iControl))   // list/thumb control
      {
        m_iSelectedLeft = m_viewControlLeft.GetSelectedItem();
        int iAction         = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          if (m_channelGroupItems->GetFileCount() == 0)
          {
            CGUIDialogOK::ShowAndGetInput(18100,18135,0,18136);
          }
          else if (m_channelLeftItems->GetFileCount() > 0)
          {
            CFileItemPtr pItemGroup   = m_channelGroupItems->Get(m_iSelectedGroup);
            CFileItemPtr pItemChannel = m_channelLeftItems->Get(m_iSelectedLeft);
            CPVRManager::GetInstance()->ChannelToGroup(pItemChannel->GetTVChannelInfoTag()->m_iChannelNum, atoi(pItemGroup->m_strPath.c_str()), m_bIsRadio);
            Update();
          }
          return true;
        }
      }
      else if (m_viewControlRight.HasControl(iControl))   // list/thumb control
      {
        m_iSelectedRight = m_viewControlRight.GetSelectedItem();
        int iAction          = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          if (m_channelRightItems->GetFileCount() > 0)
          {
            CFileItemPtr pItemChannel = m_channelRightItems->Get(m_iSelectedRight);
            CPVRManager::GetInstance()->ChannelToGroup(pItemChannel->GetTVChannelInfoTag()->m_iChannelNum, 0, m_bIsRadio);
            Update();
          }
          return true;
        }
      }
      else if (m_viewControlGroup.HasControl(iControl))   // list/thumb control
      {
        int iAction      = message.GetParam1();

        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          m_iSelectedGroup = m_viewControlGroup.GetSelectedItem();
          Update();
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVGroupManager::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  m_viewControlLeft.Reset();
  m_viewControlLeft.SetParentWindow(GetID());
  m_viewControlLeft.AddView(GetControl(CONTROL_LIST_CHANNELS_LEFT));

  m_viewControlRight.Reset();
  m_viewControlRight.SetParentWindow(GetID());
  m_viewControlRight.AddView(GetControl(CONTROL_LIST_CHANNELS_RIGHT));

  m_viewControlGroup.Reset();
  m_viewControlGroup.SetParentWindow(GetID());
  m_viewControlGroup.AddView(GetControl(CONTROL_LIST_CHANNEL_GROUPS));
}

void CGUIDialogTVGroupManager::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControlLeft.Reset();
  m_viewControlRight.Reset();
  m_viewControlGroup.Reset();
}

void CGUIDialogTVGroupManager::Update()
{
  m_CurrentGroupName = "";

  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControlLeft.SetCurrentView(CONTROL_LIST_CHANNELS_LEFT);
  m_viewControlRight.SetCurrentView(CONTROL_LIST_CHANNELS_RIGHT);
  m_viewControlGroup.SetCurrentView(CONTROL_LIST_CHANNEL_GROUPS);

  // empty the lists ready for population
  Clear();

  int groups = CPVRManager::GetInstance()->GetGroupList(m_channelGroupItems);
  m_viewControlGroup.SetItems(*m_channelGroupItems);
  m_viewControlGroup.SetSelectedItem(m_iSelectedGroup);

  if (!m_bIsRadio)
    CPVRManager::GetInstance()->GetTVChannels(m_channelLeftItems, 0, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));
  else
    CPVRManager::GetInstance()->GetRadioChannels(m_channelLeftItems, 0, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));
  m_viewControlLeft.SetItems(*m_channelLeftItems);
  m_viewControlLeft.SetSelectedItem(m_iSelectedLeft);

  if (groups > 0)
  {
    CFileItemPtr pItem = m_channelGroupItems->Get(m_viewControlGroup.GetSelectedItem());
    m_CurrentGroupName = pItem->m_strTitle;
    SET_CONTROL_LABEL(CONTROL_CURRENT_GROUP_LABEL, m_CurrentGroupName);

    if (!m_bIsRadio)
      CPVRManager::GetInstance()->GetTVChannels(m_channelRightItems, atoi(pItem->m_strPath.c_str()), false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));
    else
      CPVRManager::GetInstance()->GetRadioChannels(m_channelRightItems, atoi(pItem->m_strPath.c_str()), false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));
    m_viewControlRight.SetItems(*m_channelRightItems);
    m_viewControlRight.SetSelectedItem(m_iSelectedRight);
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogTVGroupManager::Clear()
{
  m_viewControlLeft.Clear();
  m_viewControlRight.Clear();
  m_viewControlGroup.Clear();

  m_channelLeftItems->Clear();
  m_channelRightItems->Clear();
  m_channelGroupItems->Clear();
}

void CGUIDialogTVGroupManager::SetRadio(bool IsRadio)
{
  m_bIsRadio = IsRadio;
}

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


#include "GUIDialogAccessPoints.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "ConnectionJob.h"

// defines for the controls
#define ACCESS_POINT_LABEL 1
#define ACCESS_POINT_LIST  3

//--------------------------------------------------------------
//--------------------------------------------------------------
CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
  m_doing_connection = false;
  m_connectionsFileList = new CFileItemList;
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{
  delete m_connectionsFileList;
}

void CGUIDialogAccessPoints::OnInitWindow()
{
  UpdateConnectionList();
  CGUIDialog::OnInitWindow();
}

bool CGUIDialogAccessPoints::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    // block users from doing another connection
    //  while we are already trying to connect.
    if (!m_doing_connection)
    {
      // fetch the current selected item (access point)
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), ACCESS_POINT_LIST);
      OnMessage(msg);
      int iItem = msg.GetParam1();

      ConnectionList  connections = g_application.getNetwork().GetConnections();
      CConnectionJob  *connection = new CConnectionJob(connections[iItem],
        m_ipconfig, &g_application.getKeyringManager());
      CJobManager::GetInstance().AddJob(connection, this);
      m_doing_connection = true;
    }
    return true;
  }
  else if (action.GetID() == ACTION_CONNECTIONS_REFRESH)
  {
    // msg from Network Manager when the network connection changes.
    // this is for future support for scanning for new access points.
    UpdateConnectionList();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogAccessPoints::OnBack(int actionID)
{
  // block the user from closing us if we are trying to connect.
  if (m_doing_connection)
    return false;
  else
    return CGUIDialog::OnBack(actionID);
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void CGUIDialogAccessPoints::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  // auto-close when connection job completes
  m_doing_connection = false;
  if (success)
  {
    Close(true);
  }
}

void CGUIDialogAccessPoints::UpdateConnectionList()
{
  m_connectionsFileList->Clear();

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), ACCESS_POINT_LIST);
  OnMessage(msgReset);

  int connectedItem = 0;
  ConnectionList connections = g_application.getNetwork().GetConnections();

  std::string connection_name;
  for (size_t i = 0; i < connections.size(); i++)
  {
    connection_name = connections[i]->GetName();
    CFileItemPtr item(new CFileItem(connection_name));

    if (connections[i]->GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
      connectedItem = i;

    if (connections[i]->GetType() == NETWORK_CONNECTION_TYPE_WIFI)
    {
      int strength = connections[i]->GetStrength();
      int signal   = strength / 20 + 1;

      item->SetArt("thumb", StringUtils::Format("ap-signal%d.png", signal));
      item->SetProperty("signal",     signal);
      item->SetProperty("encryption", EncryptionToString(connections[i]->GetEncryption()));
    }

    item->SetProperty("type",  ConnectionTypeToString(connections[i]->GetType()));
    item->SetProperty("state", ConnectionStateToString(connections[i]->GetState()));
 
    m_connectionsFileList->Add(item);
  }

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), ACCESS_POINT_LIST, connectedItem, 0, m_connectionsFileList);
  OnMessage(msg);
}

const char *CGUIDialogAccessPoints::ConnectionStateToString(ConnectionState state)
{
  switch (state)
  {
    case NETWORK_CONNECTION_STATE_DISCONNECTED:
      return "disconnected";
    case NETWORK_CONNECTION_STATE_CONNECTING:
      return "connecting";
    case NETWORK_CONNECTION_STATE_CONNECTED:
      return "connected";
    case NETWORK_CONNECTION_STATE_FAILURE:
      return "failure";
    case NETWORK_CONNECTION_STATE_UNKNOWN:
    default:
      return "unknown";
  }

  return "";
}

const char *CGUIDialogAccessPoints::ConnectionTypeToString(ConnectionType type)
{
  switch (type)
  {
    case NETWORK_CONNECTION_TYPE_WIRED:
      return "wired";
    case NETWORK_CONNECTION_TYPE_WIFI:
      return "wifi";
    case NETWORK_CONNECTION_TYPE_UNKNOWN:
    default:
      return "unknown";
  }

  return "";
}

const char *CGUIDialogAccessPoints::EncryptionToString(EncryptionType type)
{
  switch (type)
  {
    case NETWORK_CONNECTION_ENCRYPTION_NONE:
      return "";
    case NETWORK_CONNECTION_ENCRYPTION_WEP:
      return "wep";
    case NETWORK_CONNECTION_ENCRYPTION_WPA:
      return "wpa";
    case NETWORK_CONNECTION_ENCRYPTION_WPA2:
      return "wpa2";
    case NETWORK_CONNECTION_ENCRYPTION_IEEE8021x:
      return "wpa-rsn";
    case NETWORK_CONNECTION_ENCRYPTION_UNKNOWN:
    default:
      return "unknown";
  }

  return "";
}

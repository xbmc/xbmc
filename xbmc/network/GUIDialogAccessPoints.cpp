/*
 *      Copyright (C) 2011-2012 Team XBMC
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

#include "GUIDialogAccessPoints.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#include "utils/JobManager.h"
#include "ConnectionJob.h"

// defines for the controls
#define ACCESS_POINT_LABEL 1
#define ACCESS_POINT_LIST  3

//--------------------------------------------------------------
const std::string EncodeAccessPointParam(const std::string name, const CIPConfig &ipconfig)
{
  // encode CIPConfig structure into a string based ip connect param.
  std::string method("dhcp");
  if (ipconfig.m_method == IP_CONFIG_STATIC)
    method = "static";

  // '+' and '\t' are invalid essid characters,
  // a ' ' is valid so watch out for those in the name.
  const std::string param("name+" + name + "\t" +
      "method+"  + method             + "\t" +
      "address+" + ipconfig.m_address + "\t" +
      "netmask+" + ipconfig.m_netmask + "\t" +
      "gateway+" + ipconfig.m_gateway + "\t" +
      "nameserver+" + ipconfig.m_nameserver + "\t");
  return param;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
  m_connectionsFileList = new CFileItemList;
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{
  delete m_connectionsFileList;
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

      ConnectionList  connections = g_application.getNetworkManager().GetConnections();
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

bool CGUIDialogAccessPoints::OnMessage(CGUIMessage& message)
{
  bool result = CGUIDialog::OnMessage(message);
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_use_ipconfig = false;
      m_doing_connection = false;
      // fetch the param list
      std::string param(message.GetStringParam());

      // network apply vs network connect
      if (param.find("name+") != std::string::npos)
      {
        m_use_ipconfig = true;
        // network apply, param contains a description for connecting
        // to an access point. we want to find this access point,
        // disable the others, then inject a OnAction msg to select it.
        DecodeAccessPointParam(param);
        // change the label to show we are doing a connection.
        CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), ACCESS_POINT_LABEL);
        // <string id="33203">Connecting</string>
        msg.SetLabel(g_localizeStrings.Get(33203));
        OnMessage(msg);
      }
      UpdateConnectionList();
      // if we are doing an 'apply', then inject a click on the "selected" item.
      if (m_use_ipconfig)
        CApplicationMessenger::Get().SendAction(CAction(ACTION_SELECT_ITEM), GetID());
      break;
    }
    case GUI_MSG_WINDOW_DEINIT:
    {
    }
  }

  return result;
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
  ConnectionList connections = g_application.getNetworkManager().GetConnections();

  std::string connection_name;
  for (size_t i = 0; i < connections.size(); i++)
  {
    connection_name = connections[i]->GetName();
    CFileItemPtr item(new CFileItem(connection_name));

    if (m_use_ipconfig)
    {
      if (connection_name.find(m_ipname) != std::string::npos)
        connectedItem = i;
    }
    else
    {
      if (connections[i]->GetState() == NETWORK_CONNECTION_STATE_CONNECTED)
        connectedItem = i;
    }
    if (connections[i]->GetType() == NETWORK_CONNECTION_TYPE_WIFI)
    {
      int signal, strength = connections[i]->GetStrength();
      if (strength == 0 || strength < 20)
        signal = 1;
      else if (strength == 20 || strength < 40)
        signal = 2;
      else if (strength == 40 || strength < 60)
        signal = 3;
      else if (strength == 60 || strength < 80)
        signal = 4;
      else
        signal = 5;

        if (strength <= 20) item->SetArt("thumb", "ap-signal1.png");                                                                                                                      
        else if (strength <= 40) item->SetArt("thumb", "ap-signal2.png");                                                                                                                 
        else if (strength <= 60) item->SetArt("thumb", "ap-signal3.png");                                                                                                                 
        else if (strength <= 80) item->SetArt("thumb", "ap-signal4.png");                                                                                                                 
        else if (strength <= 100) item->SetArt("thumb", "ap-signal5.png");

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

void CGUIDialogAccessPoints::DecodeAccessPointParam(const std::string &param)
{
  // decode a string based ip connect param into a CIPConfig structure.
  std::string::size_type start;
  std::string::size_type end;

  start = param.find("name+") + sizeof("name");
  end   = param.find("\t", start);
  m_ipname = param.substr(start, end - start);
  //
  start = param.find("method+") + sizeof("method");
  end   = param.find("\t", start);
  m_ipconfig.m_method = IP_CONFIG_DHCP;
  if (param.find("static", start) != std::string::npos)
    m_ipconfig.m_method = IP_CONFIG_STATIC;
  //
  start = param.find("address+") + sizeof("address");
  end   = param.find("\t", start);
  m_ipconfig.m_address = param.substr(start, end - start);
  //
  start = param.find("netmask+") + sizeof("netmask");
  end   = param.find("\t", start);
  m_ipconfig.m_netmask = param.substr(start, end - start);
  //
  start = param.find("gateway+") + sizeof("gateway");
  end   = param.find("\t", start);
  m_ipconfig.m_gateway = param.substr(start, end - start);
  //
  start = param.find("nameserver+") + sizeof("nameserver");
  end   = param.find("\t", start);
  m_ipconfig.m_nameserver = param.substr(start, end - start);
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

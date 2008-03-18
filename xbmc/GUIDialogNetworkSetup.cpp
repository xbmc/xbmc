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
#include "GUIDialogNetworkSetup.h"
#include "GUISpinControlEx.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogFileBrowser.h"
#include "Util.h"

#define CONTROL_PROTOCOL        10
#define CONTROL_SERVER_ADDRESS  11
#define CONTROL_SERVER_BROWSE   12
#define CONTROL_PORT_NUMBER     13
#define CONTROL_USERNAME        14
#define CONTROL_PASSWORD        15
#define CONTROL_REMOTE_PATH     16
#define CONTROL_OK              18
#define CONTROL_CANCEL          19

CGUIDialogNetworkSetup::CGUIDialogNetworkSetup(void)
    : CGUIDialog(WINDOW_DIALOG_NETWORK_SETUP, "DialogNetworkSetup.xml")
{
  m_protocol = NET_PROTOCOL_SMB;
}

CGUIDialogNetworkSetup::~CGUIDialogNetworkSetup()
{
}

bool CGUIDialogNetworkSetup::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    m_confirmed = false;
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogNetworkSetup::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PROTOCOL)
      {
        m_server.Empty();
        m_path.Empty();
        m_username.Empty();
        m_password.Empty();
        OnProtocolChange();
      }
      else if (iControl == CONTROL_SERVER_BROWSE)
        OnServerBrowse();
      else if (iControl == CONTROL_SERVER_ADDRESS)
        OnServerAddress();
      else if (iControl == CONTROL_REMOTE_PATH)
        OnPath();
      else if (iControl == CONTROL_PORT_NUMBER)
        OnPort();
      else if (iControl == CONTROL_USERNAME)
        OnUserName();
      else if (iControl == CONTROL_PASSWORD)
        OnPassword();
      else if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

// \brief Show CGUIDialogNetworkSetup dialog and prompt for a new network address.
// \return True if the network address is valid, false otherwise.
bool CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(CStdString &path)
{
  CGUIDialogNetworkSetup *dialog = (CGUIDialogNetworkSetup *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NETWORK_SETUP);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPath(path);
  dialog->DoModal();
  path = dialog->ConstructPath();
  return dialog->IsConfirmed();
}

void CGUIDialogNetworkSetup::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  // Add our protocols
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (!pSpin)
    return;

  pSpin->Clear();
  pSpin->AddLabel(g_localizeStrings.Get(20171), NET_PROTOCOL_SMB);
  pSpin->AddLabel(g_localizeStrings.Get(20172), NET_PROTOCOL_XBMSP);
  pSpin->AddLabel(g_localizeStrings.Get(20173), NET_PROTOCOL_FTP);
  pSpin->AddLabel(g_localizeStrings.Get(20174), NET_PROTOCOL_DAAP);
  pSpin->AddLabel(g_localizeStrings.Get(20175), NET_PROTOCOL_UPNP);
  pSpin->AddLabel(g_localizeStrings.Get(21331), NET_PROTOCOL_TUXBOX);

  pSpin->SetValue(m_protocol);
  OnProtocolChange();
}

void CGUIDialogNetworkSetup::OnServerBrowse()
{
  // open a filebrowser dialog with the current address
  VECSHARES shares;
  CStdString path = ConstructPath();
  // get the share as the base path
  CShare share;
  CStdString basePath = path;
  CStdString tempPath;
  while (CUtil::GetParentPath(basePath, tempPath))
    basePath = tempPath;
  share.strPath = basePath;
  // don't include the user details in the share name
  CURL url(share.strPath);
  url.GetURLWithoutUserDetails(share.strName);
  shares.push_back(share);
  if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(1015), path))
  {
    SetPath(path);
    UpdateButtons();
  }
}

void CGUIDialogNetworkSetup::OnServerAddress()
{
  if (m_protocol == NET_PROTOCOL_XBMSP || m_protocol == NET_PROTOCOL_DAAP)
    CGUIDialogNumeric::ShowAndGetIPAddress(m_server, g_localizeStrings.Get(1016));
  else
    CGUIDialogKeyboard::ShowAndGetInput(m_server, g_localizeStrings.Get(1016), false);
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnPath()
{
  CGUIDialogKeyboard::ShowAndGetInput(m_path, g_localizeStrings.Get(1017), false);
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnPort()
{
  CGUIDialogNumeric::ShowAndGetNumber(m_port, g_localizeStrings.Get(1018));
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnUserName()
{
  CGUIDialogKeyboard::ShowAndGetInput(m_username, g_localizeStrings.Get(1019), false);
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnPassword()
{
  CGUIDialogKeyboard::ShowAndGetNewPassword(m_password,g_localizeStrings.Get(12326),true);
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnOK()
{
  m_confirmed = true;
  Close();
}

void CGUIDialogNetworkSetup::OnCancel()
{
  m_confirmed = false;
  Close();
}

void CGUIDialogNetworkSetup::OnProtocolChange()
{
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (!pSpin)
    return;
  m_protocol = (NET_PROTOCOL)pSpin->GetValue();
  // set defaults for the port
  if (m_protocol == NET_PROTOCOL_FTP)
    m_port = "21";
  if (m_protocol == NET_PROTOCOL_TUXBOX)
    m_port = "80";
  else if (m_protocol == NET_PROTOCOL_XBMSP)
    m_port = "1400";
  else if (m_protocol == NET_PROTOCOL_DAAP)
    m_port = "3689";

  UpdateButtons();
}

void CGUIDialogNetworkSetup::UpdateButtons()
{
  // Button labels
  if (m_protocol == NET_PROTOCOL_SMB)
  {
    SET_CONTROL_LABEL(CONTROL_SERVER_ADDRESS, 1010);  // Server name
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_SERVER_ADDRESS, 1009);  // Server Address
  }
  if (m_protocol == NET_PROTOCOL_FTP)
  {
    SET_CONTROL_LABEL(CONTROL_REMOTE_PATH, 1011);  // Remote Path
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_REMOTE_PATH, 1012);  // Shared Folder
  }
  // server
  CGUIButtonControl *server = (CGUIButtonControl *)GetControl(CONTROL_SERVER_ADDRESS);
  if (server)
  {
    server->SetLabel2(m_server);
  }
  // TODO: FIX BETTER DAAP SUPPORT
  // server browse should be disabled if we are in FTP
  if (m_server.IsEmpty() && (m_protocol == NET_PROTOCOL_FTP || m_protocol == NET_PROTOCOL_DAAP || m_protocol == NET_PROTOCOL_TUXBOX))
  {
    CONTROL_DISABLE(CONTROL_SERVER_BROWSE);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_SERVER_BROWSE);
  }
  // remote path
  CGUIButtonControl *path = (CGUIButtonControl *)GetControl(CONTROL_REMOTE_PATH);
  if (path)
  {
     path->SetLabel2(m_path);
     path->SetEnabled(m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP && m_protocol != NET_PROTOCOL_TUXBOX);
  }
  // port
  CGUIButtonControl *port = (CGUIButtonControl *)GetControl(CONTROL_PORT_NUMBER);
  if (port)
  {
    port->SetEnabled(m_protocol == NET_PROTOCOL_XBMSP || m_protocol == NET_PROTOCOL_FTP || m_protocol == NET_PROTOCOL_TUXBOX);
    port->SetLabel2(m_port);
  }
  // username
  CGUIButtonControl *username = (CGUIButtonControl *)GetControl(CONTROL_USERNAME);
  if (username)
  {
    username->SetEnabled(m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP);
    username->SetLabel2(m_username);

  }
  // password
  CGUIButtonControl *password = (CGUIButtonControl *)GetControl(CONTROL_PASSWORD);
  if (password)
  {
    CStdString asterix;
    asterix.append(m_password.size(), '*');
    password->SetLabel2(asterix);
    password->SetEnabled(m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP);
  }
}

CStdString CGUIDialogNetworkSetup::ConstructPath() const
{
  CStdString path;
  if (m_protocol == NET_PROTOCOL_SMB)
    path = "smb://";
  else if (m_protocol == NET_PROTOCOL_XBMSP)
    path = "xbms://";
  else if (m_protocol == NET_PROTOCOL_FTP)
    path = "ftp://";
  else if (m_protocol == NET_PROTOCOL_DAAP)
    path = "daap://";
  else if (m_protocol == NET_PROTOCOL_UPNP)
    path = "upnp://";
  else if (m_protocol == NET_PROTOCOL_TUXBOX)
    path = "tuxbox://";
  if (!m_username.IsEmpty())
  {
    path += m_username;
    if (!m_password.IsEmpty())
    {
      path += ":";
      path += m_password;
    }
    path += "@";
  }
  path += m_server;
  if ((m_protocol == NET_PROTOCOL_FTP && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0)
   || (m_protocol == NET_PROTOCOL_XBMSP && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0 && !m_server.IsEmpty())
   || (m_protocol == NET_PROTOCOL_DAAP && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0 && !m_server.IsEmpty())
   || (m_protocol == NET_PROTOCOL_TUXBOX && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0))
  {
    path += ":";
    path += m_port;
  }
  if (!m_path.IsEmpty())
    CUtil::AddFileToFolder(path, m_path, path);
  CUtil::AddSlashAtEnd(path);
  return path;
}

void CGUIDialogNetworkSetup::SetPath(const CStdString &path)
{
  CURL url(path);
  const CStdString &protocol = url.GetProtocol();
  if (protocol == "smb")
    m_protocol = NET_PROTOCOL_SMB;
  else if (protocol == "xbms")
    m_protocol = NET_PROTOCOL_XBMSP;
  else if (protocol == "ftp")
    m_protocol = NET_PROTOCOL_FTP;
  else if (protocol == "daap")
    m_protocol = NET_PROTOCOL_DAAP;
  else if (protocol == "upnp")
    m_protocol = NET_PROTOCOL_UPNP;
  else if (protocol == "tuxbox")
    m_protocol = NET_PROTOCOL_TUXBOX;
  else
    m_protocol = NET_PROTOCOL_SMB;  // default to smb
  m_username = url.GetUserName();
  m_password = url.GetPassWord();
  m_port.Format("%i", url.GetPort());
  m_server = url.GetHostName();
  m_path = url.GetFileName();
}


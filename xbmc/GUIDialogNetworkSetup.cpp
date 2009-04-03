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
#include "GUIDialogNetworkSetup.h"
#include "GUISpinControlEx.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogFileBrowser.h"
#include "GUIWindowManager.h"
#include "GUIEditControl.h"
#include "Util.h"
#include "URL.h"

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
  m_confirmed = false;
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
        OnEditChanged(iControl, m_server);
      else if (iControl == CONTROL_REMOTE_PATH)
        OnEditChanged(iControl, m_path);
      else if (iControl == CONTROL_PORT_NUMBER)
        OnEditChanged(iControl, m_port);
      else if (iControl == CONTROL_USERNAME)
        OnEditChanged(iControl, m_username);
      else if (iControl == CONTROL_PASSWORD)
        OnEditChanged(iControl, m_password);
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
  // replace our buttons with edits
  ChangeButtonToEdit(CONTROL_SERVER_ADDRESS);
  ChangeButtonToEdit(CONTROL_REMOTE_PATH);
  ChangeButtonToEdit(CONTROL_USERNAME);
  ChangeButtonToEdit(CONTROL_PORT_NUMBER);
  ChangeButtonToEdit(CONTROL_PASSWORD);

  // start as unconfirmed
  m_confirmed = false;

  CGUIDialog::OnInitWindow();
  // Add our protocols
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (!pSpin)
    return;

  pSpin->Clear();
  pSpin->AddLabel(g_localizeStrings.Get(20171), NET_PROTOCOL_SMB);
  pSpin->AddLabel(g_localizeStrings.Get(21331), NET_PROTOCOL_TUXBOX);
  pSpin->AddLabel(g_localizeStrings.Get(20172), NET_PROTOCOL_XBMSP);
  pSpin->AddLabel(g_localizeStrings.Get(20301), NET_PROTOCOL_HTTPS);
  pSpin->AddLabel(g_localizeStrings.Get(20300), NET_PROTOCOL_HTTP);
  pSpin->AddLabel(g_localizeStrings.Get(20173), NET_PROTOCOL_FTP);
  pSpin->AddLabel(g_localizeStrings.Get(20174), NET_PROTOCOL_DAAP);
  pSpin->AddLabel(g_localizeStrings.Get(20175), NET_PROTOCOL_UPNP);

  pSpin->SetValue(m_protocol);
  OnProtocolChange();
}

void CGUIDialogNetworkSetup::OnServerBrowse()
{
  // open a filebrowser dialog with the current address
  VECSOURCES shares;
  CStdString path = ConstructPath();
  // get the share as the base path
  CMediaSource share;
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
  if (m_protocol == NET_PROTOCOL_HTTP)
    m_port = "80";
  if (m_protocol == NET_PROTOCOL_HTTPS)
    m_port = "443";
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
  // Address label
  SET_CONTROL_LABEL2(CONTROL_SERVER_ADDRESS, m_server);
  if (m_protocol == NET_PROTOCOL_SMB)
  {
    SET_CONTROL_LABEL(CONTROL_SERVER_ADDRESS, 1010);  // Server name
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_SERVER_ADDRESS, 1009);  // Server Address
  }
  if (m_protocol == NET_PROTOCOL_XBMSP || m_protocol == NET_PROTOCOL_DAAP)
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SERVER_ADDRESS, CGUIEditControl::INPUT_TYPE_IPADDRESS, 1016);
  else
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SERVER_ADDRESS, CGUIEditControl::INPUT_TYPE_TEXT, 1016);
  // remote path
  SET_CONTROL_LABEL2(CONTROL_REMOTE_PATH, m_path);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_REMOTE_PATH, m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP && m_protocol != NET_PROTOCOL_TUXBOX);
  if (m_protocol == NET_PROTOCOL_FTP || m_protocol == NET_PROTOCOL_HTTP || m_protocol == NET_PROTOCOL_HTTPS)
  {
    SET_CONTROL_LABEL(CONTROL_REMOTE_PATH, 1011);  // Remote Path
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_REMOTE_PATH, 1012);  // Shared Folder
  }
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_REMOTE_PATH, CGUIEditControl::INPUT_TYPE_TEXT, 1017);

  // username
  SET_CONTROL_LABEL2(CONTROL_USERNAME, m_username);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_USERNAME, m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP);
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_USERNAME, CGUIEditControl::INPUT_TYPE_TEXT, 1019);

  // port
  SET_CONTROL_LABEL2(CONTROL_PORT_NUMBER, m_port);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PORT_NUMBER, m_protocol == NET_PROTOCOL_XBMSP ||
                                                   m_protocol == NET_PROTOCOL_FTP ||
                                                   m_protocol == NET_PROTOCOL_HTTP ||
                                                   m_protocol == NET_PROTOCOL_HTTPS ||
                                                   m_protocol == NET_PROTOCOL_TUXBOX ||
                                                   m_protocol == NET_PROTOCOL_DAAP);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PORT_NUMBER, CGUIEditControl::INPUT_TYPE_NUMBER, 1018);

  // password
  SET_CONTROL_LABEL2(CONTROL_PASSWORD, m_password);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PASSWORD, m_protocol != NET_PROTOCOL_DAAP && m_protocol != NET_PROTOCOL_UPNP);
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PASSWORD, CGUIEditControl::INPUT_TYPE_PASSWORD, 12326);

  // TODO: FIX BETTER DAAP SUPPORT
  // server browse should be disabled if we are in DAAP, FTP, HTTP, HTTPS or TUXBOX
  CONTROL_ENABLE_ON_CONDITION(CONTROL_SERVER_BROWSE, !m_server.IsEmpty() || !(m_protocol == NET_PROTOCOL_FTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTPS ||
                                                                              m_protocol == NET_PROTOCOL_DAAP ||
                                                                              m_protocol == NET_PROTOCOL_TUXBOX));
}

CStdString CGUIDialogNetworkSetup::ConstructPath() const
{
  CURL url;
  if (m_protocol == NET_PROTOCOL_SMB)
    url.SetProtocol("smb");
  else if (m_protocol == NET_PROTOCOL_XBMSP)
    url.SetProtocol("xbms");
  else if (m_protocol == NET_PROTOCOL_FTP)
    url.SetProtocol("ftp");
  else if (m_protocol == NET_PROTOCOL_HTTP)
    url.SetProtocol("http");
  else if (m_protocol == NET_PROTOCOL_HTTPS)
    url.SetProtocol("https");
  else if (m_protocol == NET_PROTOCOL_DAAP)
    url.SetProtocol("daap");
  else if (m_protocol == NET_PROTOCOL_UPNP)
    url.SetProtocol("upnp");
  else if (m_protocol == NET_PROTOCOL_TUXBOX)
    url.SetProtocol("tuxbox");
  if (!m_username.IsEmpty() && !m_server.IsEmpty())
  {
    url.SetUserName(m_username);
    if (!m_password.IsEmpty())
      url.SetPassword(m_password);
    url.SetHostName(m_server);
  }
  if (((m_protocol == NET_PROTOCOL_FTP) ||
       (m_protocol == NET_PROTOCOL_HTTP) || 
       (m_protocol == NET_PROTOCOL_HTTPS) ||
       (m_protocol == NET_PROTOCOL_XBMSP && !m_server.IsEmpty()) ||
       (m_protocol == NET_PROTOCOL_DAAP && !m_server.IsEmpty()) ||
       (m_protocol == NET_PROTOCOL_TUXBOX))
      && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0)
  {
    url.SetPort(atoi(m_port));
  }
  if (!m_path.IsEmpty())
    url.SetFileName(m_path);
  CStdString path;
  url.GetURL(path);
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
  else if (protocol == "http")
    m_protocol = NET_PROTOCOL_HTTP;
  else if (protocol == "https")
    m_protocol = NET_PROTOCOL_HTTPS;
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


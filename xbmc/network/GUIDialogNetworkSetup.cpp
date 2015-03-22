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

#include "GUIDialogNetworkSetup.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIEditControl.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"

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
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogNetworkSetup::~CGUIDialogNetworkSetup()
{
}

bool CGUIDialogNetworkSetup::OnBack(int actionID)
{
  m_confirmed = false;
  return CGUIDialog::OnBack(actionID);
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
        m_server.clear();
        m_path.clear();
        m_username.clear();
        m_password.clear();
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
bool CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(std::string &path)
{
  CGUIDialogNetworkSetup *dialog = (CGUIDialogNetworkSetup *)g_windowManager.GetWindow(WINDOW_DIALOG_NETWORK_SETUP);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPath(path);
  dialog->DoModal();
  path = dialog->ConstructPath();
  return dialog->IsConfirmed();
}

void CGUIDialogNetworkSetup::OnInitWindow()
{
  // start as unconfirmed
  m_confirmed = false;

  CGUIDialog::OnInitWindow();

  // Add our protocols
  std::vector< std::pair<std::string, int> > labels;
#ifdef HAS_FILESYSTEM_SMB
  labels.push_back(make_pair(g_localizeStrings.Get(20171), NET_PROTOCOL_SMB));
#endif
  labels.push_back(make_pair(g_localizeStrings.Get(20301), NET_PROTOCOL_HTTPS));
  labels.push_back(make_pair(g_localizeStrings.Get(20300), NET_PROTOCOL_HTTP));
  labels.push_back(make_pair(g_localizeStrings.Get(20254), NET_PROTOCOL_DAVS));
  labels.push_back(make_pair(g_localizeStrings.Get(20253), NET_PROTOCOL_DAV));
  labels.push_back(make_pair(g_localizeStrings.Get(20173), NET_PROTOCOL_FTP));
  labels.push_back(make_pair(g_localizeStrings.Get(20175), NET_PROTOCOL_UPNP));
  labels.push_back(make_pair(g_localizeStrings.Get(20304), NET_PROTOCOL_RSS));
#ifdef HAS_FILESYSTEM_NFS
  labels.push_back(make_pair(g_localizeStrings.Get(20259), NET_PROTOCOL_NFS));
#endif
#ifdef HAS_FILESYSTEM_SFTP
  labels.push_back(make_pair(g_localizeStrings.Get(20260), NET_PROTOCOL_SFTP));
#endif

  SET_CONTROL_LABELS(CONTROL_PROTOCOL, m_protocol, &labels);
  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnDeinitWindow(int nextWindowID)
{
  // clear protocol spinner
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_PROTOCOL);
  OnMessage(msg);

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIDialogNetworkSetup::OnServerBrowse()
{
  // open a filebrowser dialog with the current address
  VECSOURCES shares;
  std::string path = ConstructPath();
  // get the share as the base path
  CMediaSource share;
  std::string basePath = path;
  std::string tempPath;
  while (URIUtils::GetParentPath(basePath, tempPath))
    basePath = tempPath;
  share.strPath = basePath;
  // don't include the user details in the share name
  CURL url(share.strPath);
  share.strName = url.GetWithoutUserDetails();
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
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_PROTOCOL);
  if (!OnMessage(msg))
    return;
  m_protocol = (NET_PROTOCOL)msg.GetParam1();
  // set defaults for the port
  if (m_protocol == NET_PROTOCOL_FTP)
    m_port = "21";
  else if (m_protocol == NET_PROTOCOL_HTTP || 
	   m_protocol == NET_PROTOCOL_RSS || 
	   m_protocol == NET_PROTOCOL_DAV)
    m_port = "80";
  else if (m_protocol == NET_PROTOCOL_HTTPS || m_protocol == NET_PROTOCOL_DAVS)
    m_port = "443";
  else if (m_protocol == NET_PROTOCOL_SFTP)
    m_port = "22";
  else
    m_port = "0";

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
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_SERVER_ADDRESS, CGUIEditControl::INPUT_TYPE_TEXT, 1016);
  // remote path
  SET_CONTROL_LABEL2(CONTROL_REMOTE_PATH, m_path);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_REMOTE_PATH, m_protocol != NET_PROTOCOL_UPNP);
  if (m_protocol == NET_PROTOCOL_FTP ||
      m_protocol == NET_PROTOCOL_HTTP ||
      m_protocol == NET_PROTOCOL_HTTPS ||
      m_protocol == NET_PROTOCOL_RSS ||
      m_protocol == NET_PROTOCOL_DAV ||
      m_protocol == NET_PROTOCOL_DAVS||
      m_protocol == NET_PROTOCOL_SFTP||
      m_protocol == NET_PROTOCOL_NFS)
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
  CONTROL_ENABLE_ON_CONDITION(CONTROL_USERNAME, m_protocol != NET_PROTOCOL_UPNP &&
                                                m_protocol != NET_PROTOCOL_NFS);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_USERNAME, CGUIEditControl::INPUT_TYPE_TEXT, 1019);

  // port
  SET_CONTROL_LABEL2(CONTROL_PORT_NUMBER, m_port);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PORT_NUMBER, m_protocol == NET_PROTOCOL_FTP ||
                                                   m_protocol == NET_PROTOCOL_HTTP ||
                                                   m_protocol == NET_PROTOCOL_HTTPS ||
                                                   m_protocol == NET_PROTOCOL_DAV ||
                                                   m_protocol == NET_PROTOCOL_DAVS ||
                                                   m_protocol == NET_PROTOCOL_RSS ||
                                                   m_protocol == NET_PROTOCOL_SFTP);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PORT_NUMBER, CGUIEditControl::INPUT_TYPE_NUMBER, 1018);

  // password
  SET_CONTROL_LABEL2(CONTROL_PASSWORD, m_password);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PASSWORD, m_protocol != NET_PROTOCOL_UPNP &&
                                                m_protocol != NET_PROTOCOL_NFS);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PASSWORD, CGUIEditControl::INPUT_TYPE_PASSWORD, 12326);

  // server browse should be disabled if we are in FTP, HTTP, HTTPS, RSS, DAV or DAVS
  CONTROL_ENABLE_ON_CONDITION(CONTROL_SERVER_BROWSE, !m_server.empty() || !(m_protocol == NET_PROTOCOL_FTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTPS ||
                                                                              m_protocol == NET_PROTOCOL_DAV ||
                                                                              m_protocol == NET_PROTOCOL_DAVS ||
                                                                              m_protocol == NET_PROTOCOL_RSS ||
                                                                              m_protocol == NET_PROTOCOL_SFTP));
}

std::string CGUIDialogNetworkSetup::ConstructPath() const
{
  CURL url;
  if (m_protocol == NET_PROTOCOL_SMB)
    url.SetProtocol("smb");
  else if (m_protocol == NET_PROTOCOL_FTP)
    url.SetProtocol("ftp");
  else if (m_protocol == NET_PROTOCOL_HTTP)
    url.SetProtocol("http");
  else if (m_protocol == NET_PROTOCOL_HTTPS)
    url.SetProtocol("https");
  else if (m_protocol == NET_PROTOCOL_DAV)
    url.SetProtocol("dav");
  else if (m_protocol == NET_PROTOCOL_DAVS)
    url.SetProtocol("davs");
  else if (m_protocol == NET_PROTOCOL_UPNP)
    url.SetProtocol("upnp");
  else if (m_protocol == NET_PROTOCOL_RSS)
    url.SetProtocol("rss");
  else if (m_protocol == NET_PROTOCOL_NFS)
    url.SetProtocol("nfs");
  else if (m_protocol == NET_PROTOCOL_SFTP)
    url.SetProtocol("sftp");
    
  if (!m_username.empty())
  {
    url.SetUserName(m_username);
    if (!m_password.empty())
      url.SetPassword(m_password);
  }
  if(!m_server.empty())
    url.SetHostName(m_server);
  if (((m_protocol == NET_PROTOCOL_FTP) ||
       (m_protocol == NET_PROTOCOL_HTTP) ||
       (m_protocol == NET_PROTOCOL_HTTPS) ||
       (m_protocol == NET_PROTOCOL_DAV) ||
       (m_protocol == NET_PROTOCOL_DAVS) ||
       (m_protocol == NET_PROTOCOL_RSS) ||
       (m_protocol == NET_PROTOCOL_SFTP) ||
       (m_protocol == NET_PROTOCOL_NFS))
      && !m_port.empty() && atoi(m_port.c_str()) > 0)
  {
    url.SetPort(atoi(m_port.c_str()));
  }
  if (!m_path.empty())
    url.SetFileName(m_path);
  return url.Get();
}

void CGUIDialogNetworkSetup::SetPath(const std::string &path)
{
  CURL url(path);
  if (url.IsProtocol("smb"))
    m_protocol = NET_PROTOCOL_SMB;
  else if (url.IsProtocol("ftp"))
    m_protocol = NET_PROTOCOL_FTP;
  else if (url.IsProtocol("http"))
    m_protocol = NET_PROTOCOL_HTTP;
  else if (url.IsProtocol("https"))
    m_protocol = NET_PROTOCOL_HTTPS;
  else if (url.IsProtocol("dav"))
    m_protocol = NET_PROTOCOL_DAV;
  else if (url.IsProtocol("davs"))
    m_protocol = NET_PROTOCOL_DAVS;
  else if (url.IsProtocol("upnp"))
    m_protocol = NET_PROTOCOL_UPNP;
  else if (url.IsProtocol("rss"))
    m_protocol = NET_PROTOCOL_RSS;
  else if (url.IsProtocol("nfs"))
    m_protocol = NET_PROTOCOL_NFS;
  else if (url.IsProtocol("sftp") || url.IsProtocol("ssh"))
    m_protocol = NET_PROTOCOL_SFTP;
  else
    m_protocol = NET_PROTOCOL_SMB;  // default to smb
  m_username = url.GetUserName();
  m_password = url.GetPassWord();
  m_port = StringUtils::Format("%i", url.GetPort());
  m_server = url.GetHostName();
  m_path = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(m_path);
}


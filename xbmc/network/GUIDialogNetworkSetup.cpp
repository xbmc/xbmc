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

#include "GUIDialogNetworkSetup.h"
#include "guilib/GUISpinControlEx.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIEditControl.h"
#include "utils/URIUtils.h"
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
  CGUIDialogNetworkSetup *dialog = (CGUIDialogNetworkSetup *)g_windowManager.GetWindow(WINDOW_DIALOG_NETWORK_SETUP);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPath(path);
  dialog->DoModal();
  path = dialog->ConstructPath();
  return dialog->IsConfirmed();
}

void CGUIDialogNetworkSetup::OnWindowLoaded()
{
  // replace our buttons with edits
  ChangeButtonToEdit(CONTROL_SERVER_ADDRESS);
  ChangeButtonToEdit(CONTROL_REMOTE_PATH);
  ChangeButtonToEdit(CONTROL_USERNAME);
  ChangeButtonToEdit(CONTROL_PORT_NUMBER);
  ChangeButtonToEdit(CONTROL_PASSWORD);

  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogNetworkSetup::OnInitWindow()
{
  // start as unconfirmed
  m_confirmed = false;

  CGUIDialog::OnInitWindow();
  // Add our protocols
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (!pSpin)
    return;

  pSpin->Clear();
#ifdef HAS_FILESYSTEM_SMB
  pSpin->AddLabel(g_localizeStrings.Get(20171), NET_PROTOCOL_SMB);
#endif
  pSpin->AddLabel(g_localizeStrings.Get(20256), NET_PROTOCOL_HTSP);
  pSpin->AddLabel(g_localizeStrings.Get(20257), NET_PROTOCOL_VTP);
#ifdef HAS_MYSQL
  pSpin->AddLabel(g_localizeStrings.Get(20258), NET_PROTOCOL_MYTH);
#endif
  pSpin->AddLabel(g_localizeStrings.Get(21331), NET_PROTOCOL_TUXBOX);
  pSpin->AddLabel(g_localizeStrings.Get(20301), NET_PROTOCOL_HTTPS);
  pSpin->AddLabel(g_localizeStrings.Get(20300), NET_PROTOCOL_HTTP);
  pSpin->AddLabel(g_localizeStrings.Get(20254), NET_PROTOCOL_DAVS);
  pSpin->AddLabel(g_localizeStrings.Get(20253), NET_PROTOCOL_DAV);
  pSpin->AddLabel(g_localizeStrings.Get(20173), NET_PROTOCOL_FTP);
  pSpin->AddLabel(g_localizeStrings.Get(20174), NET_PROTOCOL_DAAP);
  pSpin->AddLabel(g_localizeStrings.Get(20175), NET_PROTOCOL_UPNP);
  pSpin->AddLabel(g_localizeStrings.Get(20304), NET_PROTOCOL_RSS);
#ifdef HAS_FILESYSTEM_NFS
  pSpin->AddLabel(g_localizeStrings.Get(20259), NET_PROTOCOL_NFS);
#endif
#ifdef HAS_FILESYSTEM_SFTP
  pSpin->AddLabel(g_localizeStrings.Get(20260), NET_PROTOCOL_SFTP);
#endif
#ifdef HAS_FILESYSTEM_AFP
  pSpin->AddLabel(g_localizeStrings.Get(20261), NET_PROTOCOL_AFP);
#endif

  pSpin->SetValue(m_protocol);
  OnProtocolChange();
}

void CGUIDialogNetworkSetup::OnDeinitWindow(int nextWindowID)
{
  // clear protocol spinner
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (pSpin)
    pSpin->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
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
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_PROTOCOL);
  if (!pSpin)
    return;
  m_protocol = (NET_PROTOCOL)pSpin->GetValue();
  // set defaults for the port
  if (m_protocol == NET_PROTOCOL_FTP)
    m_port = "21";
  else if (m_protocol == NET_PROTOCOL_HTTP || 
	   m_protocol == NET_PROTOCOL_RSS || 
	   m_protocol == NET_PROTOCOL_TUXBOX || 
	   m_protocol == NET_PROTOCOL_DAV)
    m_port = "80";
  else if (m_protocol == NET_PROTOCOL_HTTPS || m_protocol == NET_PROTOCOL_DAVS)
    m_port = "443";
  else if (m_protocol == NET_PROTOCOL_DAAP)
    m_port = "3689";
  else if (m_protocol == NET_PROTOCOL_HTSP)
    m_port = "9982";
  else if (m_protocol == NET_PROTOCOL_VTP)
    m_port = "2004";
  else if (m_protocol == NET_PROTOCOL_MYTH)
    m_port = "6543";
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
  if (m_protocol == NET_PROTOCOL_DAAP)
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SERVER_ADDRESS, CGUIEditControl::INPUT_TYPE_IPADDRESS, 1016);
  else
    SendMessage(GUI_MSG_SET_TYPE, CONTROL_SERVER_ADDRESS, CGUIEditControl::INPUT_TYPE_TEXT, 1016);
  // remote path
  SET_CONTROL_LABEL2(CONTROL_REMOTE_PATH, m_path);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_REMOTE_PATH, m_protocol != NET_PROTOCOL_DAAP &&
                                                   m_protocol != NET_PROTOCOL_UPNP &&
                                                   m_protocol != NET_PROTOCOL_TUXBOX &&
                                                   m_protocol != NET_PROTOCOL_HTSP &&
                                                   m_protocol != NET_PROTOCOL_VTP &&
                                                   m_protocol != NET_PROTOCOL_MYTH);
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
  CONTROL_ENABLE_ON_CONDITION(CONTROL_USERNAME, m_protocol != NET_PROTOCOL_DAAP &&
                                                m_protocol != NET_PROTOCOL_VTP &&
                                                m_protocol != NET_PROTOCOL_UPNP &&
                                                m_protocol != NET_PROTOCOL_NFS);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_USERNAME, CGUIEditControl::INPUT_TYPE_TEXT, 1019);

  // port
  SET_CONTROL_LABEL2(CONTROL_PORT_NUMBER, m_port);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PORT_NUMBER, m_protocol == NET_PROTOCOL_FTP ||
                                                   m_protocol == NET_PROTOCOL_HTTP ||
                                                   m_protocol == NET_PROTOCOL_HTTPS ||
                                                   m_protocol == NET_PROTOCOL_DAV ||
                                                   m_protocol == NET_PROTOCOL_DAVS ||
                                                   m_protocol == NET_PROTOCOL_TUXBOX ||
                                                   m_protocol == NET_PROTOCOL_HTSP ||
                                                   m_protocol == NET_PROTOCOL_VTP ||
                                                   m_protocol == NET_PROTOCOL_MYTH ||
                                                   m_protocol == NET_PROTOCOL_RSS ||
                                                   m_protocol == NET_PROTOCOL_DAAP ||
                                                   m_protocol == NET_PROTOCOL_SFTP);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PORT_NUMBER, CGUIEditControl::INPUT_TYPE_NUMBER, 1018);

  // password
  SET_CONTROL_LABEL2(CONTROL_PASSWORD, m_password);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_PASSWORD, m_protocol != NET_PROTOCOL_DAAP &&
                                                m_protocol != NET_PROTOCOL_VTP &&
                                                m_protocol != NET_PROTOCOL_UPNP &&
                                                m_protocol != NET_PROTOCOL_NFS);

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_PASSWORD, CGUIEditControl::INPUT_TYPE_PASSWORD, 12326);

  // TODO: FIX BETTER DAAP SUPPORT
  // server browse should be disabled if we are in DAAP, FTP, HTTP, HTTPS, RSS, HTSP, VTP, TUXBOX, DAV or DAVS
  CONTROL_ENABLE_ON_CONDITION(CONTROL_SERVER_BROWSE, !m_server.IsEmpty() || !(m_protocol == NET_PROTOCOL_FTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTP ||
                                                                              m_protocol == NET_PROTOCOL_HTTPS ||
                                                                              m_protocol == NET_PROTOCOL_DAV ||
                                                                              m_protocol == NET_PROTOCOL_DAVS ||
                                                                              m_protocol == NET_PROTOCOL_DAAP ||
                                                                              m_protocol == NET_PROTOCOL_RSS ||
                                                                              m_protocol == NET_PROTOCOL_HTSP ||
                                                                              m_protocol == NET_PROTOCOL_VTP ||
                                                                              m_protocol == NET_PROTOCOL_MYTH ||
                                                                              m_protocol == NET_PROTOCOL_TUXBOX||
                                                                              m_protocol == NET_PROTOCOL_SFTP ||
                                                                              m_protocol == NET_PROTOCOL_AFP));
}

CStdString CGUIDialogNetworkSetup::ConstructPath() const
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
  else if (m_protocol == NET_PROTOCOL_DAAP)
    url.SetProtocol("daap");
  else if (m_protocol == NET_PROTOCOL_UPNP)
    url.SetProtocol("upnp");
  else if (m_protocol == NET_PROTOCOL_TUXBOX)
    url.SetProtocol("tuxbox");
  else if (m_protocol == NET_PROTOCOL_RSS)
    url.SetProtocol("rss");
  else if (m_protocol == NET_PROTOCOL_HTSP)
    url.SetProtocol("htsp");
  else if (m_protocol == NET_PROTOCOL_VTP)
    url.SetProtocol("vtp");
  else if (m_protocol == NET_PROTOCOL_MYTH)
    url.SetProtocol("myth");
  else if (m_protocol == NET_PROTOCOL_NFS)
    url.SetProtocol("nfs");
  else if (m_protocol == NET_PROTOCOL_SFTP)
    url.SetProtocol("sftp");
  else if (m_protocol == NET_PROTOCOL_AFP)
    url.SetProtocol("afp");
    
  if (!m_username.IsEmpty())
  {
    url.SetUserName(m_username);
    if (!m_password.IsEmpty())
      url.SetPassword(m_password);
  }
  if(!m_server.IsEmpty())
    url.SetHostName(m_server);
  if (((m_protocol == NET_PROTOCOL_FTP) ||
       (m_protocol == NET_PROTOCOL_HTTP) ||
       (m_protocol == NET_PROTOCOL_HTTPS) ||
       (m_protocol == NET_PROTOCOL_DAV) ||
       (m_protocol == NET_PROTOCOL_DAVS) ||
       (m_protocol == NET_PROTOCOL_RSS) ||
       (m_protocol == NET_PROTOCOL_DAAP && !m_server.IsEmpty()) ||
       (m_protocol == NET_PROTOCOL_HTSP) ||
       (m_protocol == NET_PROTOCOL_VTP) ||
       (m_protocol == NET_PROTOCOL_MYTH) ||
       (m_protocol == NET_PROTOCOL_TUXBOX) ||
       (m_protocol == NET_PROTOCOL_SFTP) ||
       (m_protocol == NET_PROTOCOL_NFS))
      && !m_port.IsEmpty() && atoi(m_port.c_str()) > 0)
  {
    url.SetPort(atoi(m_port));
  }
  if (!m_path.IsEmpty())
    url.SetFileName(m_path);
  return url.Get();
}

void CGUIDialogNetworkSetup::SetPath(const CStdString &path)
{
  CURL url(path);
  const CStdString &protocol = url.GetProtocol();
  if (protocol == "smb")
    m_protocol = NET_PROTOCOL_SMB;
  else if (protocol == "ftp")
    m_protocol = NET_PROTOCOL_FTP;
  else if (protocol == "http")
    m_protocol = NET_PROTOCOL_HTTP;
  else if (protocol == "https")
    m_protocol = NET_PROTOCOL_HTTPS;
  else if (protocol == "dav")
    m_protocol = NET_PROTOCOL_DAV;
  else if (protocol == "davs")
    m_protocol = NET_PROTOCOL_DAVS;
  else if (protocol == "daap")
    m_protocol = NET_PROTOCOL_DAAP;
  else if (protocol == "upnp")
    m_protocol = NET_PROTOCOL_UPNP;
  else if (protocol == "tuxbox")
    m_protocol = NET_PROTOCOL_TUXBOX;
  else if (protocol == "htsp")
    m_protocol = NET_PROTOCOL_HTSP;
  else if (protocol == "vtp")
    m_protocol = NET_PROTOCOL_VTP;
  else if (protocol == "myth")
    m_protocol = NET_PROTOCOL_MYTH;
  else if (protocol == "rss")
    m_protocol = NET_PROTOCOL_RSS;
  else if (protocol == "nfs")
    m_protocol = NET_PROTOCOL_NFS;
  else if (protocol == "sftp" || protocol == "ssh")
    m_protocol = NET_PROTOCOL_SFTP;
  else if (protocol == "afp")
    m_protocol = NET_PROTOCOL_AFP;
  else
    m_protocol = NET_PROTOCOL_SMB;  // default to smb
  m_username = url.GetUserName();
  m_password = url.GetPassWord();
  m_port.Format("%i", url.GetPort());
  m_server = url.GetHostName();
  m_path = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(m_path);
}


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

#include <utility>

#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/windows/GUIControlSettings.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#define CONTROL_OK              28
#define CONTROL_CANCEL          29

#define SETTING_PROTOCOL        "protocol"
#define SETTING_SERVER_ADDRESS  "serveraddress"
#define SETTING_SERVER_BROWSE   "serverbrowse"
#define SETTING_PORT_NUMBER     "portnumber"
#define SETTING_USERNAME        "username"
#define SETTING_PASSWORD        "password"
#define SETTING_REMOTE_PATH     "remotepath"

CGUIDialogNetworkSetup::CGUIDialogNetworkSetup(void)
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_NETWORK_SETUP, "DialogSettings.xml")
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
  return CGUIDialogSettingsManualBase::OnBack(actionID);
}

bool CGUIDialogNetworkSetup::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_OK)
      {
        OnOK();
        return true;
      }
      else if (iControl == CONTROL_CANCEL)
      {
        OnCancel();
        return true;
      }
    }
    break;
  }
  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogNetworkSetup::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_PROTOCOL)
  {
    m_server.clear();
    m_path.clear();
    m_username.clear();
    m_password.clear();
    OnProtocolChange();
  }
  else if (settingId == SETTING_SERVER_ADDRESS)
    m_server = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_REMOTE_PATH)
    m_path = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_PORT_NUMBER)
    m_port = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_USERNAME)
    m_username = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_PASSWORD)
    m_password = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
}

void CGUIDialogNetworkSetup::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_SERVER_BROWSE)
    OnServerBrowse();
}

// \brief Show CGUIDialogNetworkSetup dialog and prompt for a new network address.
// \return True if the network address is valid, false otherwise.
bool CGUIDialogNetworkSetup::ShowAndGetNetworkAddress(std::string &path)
{
  CGUIDialogNetworkSetup *dialog = g_windowManager.GetWindow<CGUIDialogNetworkSetup>(WINDOW_DIALOG_NETWORK_SETUP);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPath(path);
  dialog->Open();
  path = dialog->ConstructPath();
  return dialog->IsConfirmed();
}

void CGUIDialogNetworkSetup::OnInitWindow()
{
  // start as unconfirmed
  m_confirmed = false;

  CGUIDialogSettingsManualBase::OnInitWindow();

  UpdateButtons();
}

void CGUIDialogNetworkSetup::OnDeinitWindow(int nextWindowID)
{
  // clear protocol spinner
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_PROTOCOL);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), settingControl->GetID());
    OnMessage(msg);
  }

  CGUIDialogSettingsManualBase::OnDeinitWindow(nextWindowID);
}

void CGUIDialogNetworkSetup::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(1007);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
}

void CGUIDialogNetworkSetup::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("networksetupsettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogNetworkSetup: unable to setup settings");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogNetworkSetup: unable to setup settings");
    return;
  }

  // Add our protocols
  TranslatableIntegerSettingOptions labels;
#ifdef HAS_FILESYSTEM_SMB
  labels.push_back(std::make_pair(20171, NET_PROTOCOL_SMB));
#endif
  labels.push_back(std::make_pair(20301, NET_PROTOCOL_HTTPS));
  labels.push_back(std::make_pair(20300, NET_PROTOCOL_HTTP));
  labels.push_back(std::make_pair(20254, NET_PROTOCOL_DAVS));
  labels.push_back(std::make_pair(20253, NET_PROTOCOL_DAV));
  labels.push_back(std::make_pair(20173, NET_PROTOCOL_FTP));
  labels.push_back(std::make_pair(20175, NET_PROTOCOL_UPNP));
  labels.push_back(std::make_pair(20304, NET_PROTOCOL_RSS));
#ifdef HAS_FILESYSTEM_NFS
  labels.push_back(std::make_pair(20259, NET_PROTOCOL_NFS));
#endif
#ifdef HAS_FILESYSTEM_SFTP
  labels.push_back(std::make_pair(20260, NET_PROTOCOL_SFTP));
#endif

  AddSpinner(group, SETTING_PROTOCOL, 1008, 0, m_protocol, labels);
  AddEdit(group, SETTING_SERVER_ADDRESS, 1010, 0, m_server, true);
  std::shared_ptr<CSettingAction> subsetting = AddButton(group, SETTING_SERVER_BROWSE, 1024, 0, "", false);
  if (subsetting != NULL)
    subsetting->SetParent(SETTING_SERVER_ADDRESS);

  AddEdit(group, SETTING_REMOTE_PATH, 1012, 0, m_path, true);
  AddEdit(group, SETTING_PORT_NUMBER, 1013, 0, m_port, true);
  AddEdit(group, SETTING_USERNAME, 1014, 0, m_username, true);
  AddEdit(group, SETTING_PASSWORD, 15052, 0, m_password, true, true);
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
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_PROTOCOL);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), settingControl->GetID());
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
}

void CGUIDialogNetworkSetup::UpdateButtons()
{
  // Address label
  BaseSettingControlPtr addressControl = GetSettingControl(SETTING_SERVER_ADDRESS);
  if (addressControl != NULL && addressControl->GetControl() != NULL)
  {
    int addressControlID = addressControl->GetID();
    SET_CONTROL_LABEL2(addressControlID, m_server);
    if (m_protocol == NET_PROTOCOL_SMB)
    {
      SET_CONTROL_LABEL(addressControlID, 1010);  // Server name
    }
    else
    {
      SET_CONTROL_LABEL(addressControlID, 1009);  // Server Address
    }
    SendMessage(GUI_MSG_SET_TYPE, addressControlID, CGUIEditControl::INPUT_TYPE_TEXT, 1016);
  }

  // remote path
  BaseSettingControlPtr pathControl = GetSettingControl(SETTING_REMOTE_PATH);
  if (pathControl != NULL && pathControl->GetControl() != NULL)
  {
    int pathControlID = pathControl->GetID();
    SET_CONTROL_LABEL2(pathControlID, m_path);
    CONTROL_ENABLE_ON_CONDITION(pathControlID, m_protocol != NET_PROTOCOL_UPNP);
    if (m_protocol == NET_PROTOCOL_FTP ||
        m_protocol == NET_PROTOCOL_HTTP ||
        m_protocol == NET_PROTOCOL_HTTPS ||
        m_protocol == NET_PROTOCOL_RSS ||
        m_protocol == NET_PROTOCOL_DAV ||
        m_protocol == NET_PROTOCOL_DAVS||
        m_protocol == NET_PROTOCOL_SFTP||
        m_protocol == NET_PROTOCOL_NFS)
    {
      SET_CONTROL_LABEL(pathControlID, 1011);  // Remote Path
    }
    else
    {
      SET_CONTROL_LABEL(pathControlID, 1012);  // Shared Folder
    }
    SendMessage(GUI_MSG_SET_TYPE, pathControlID, CGUIEditControl::INPUT_TYPE_TEXT, 1017);
  }

  // username
  BaseSettingControlPtr userControl = GetSettingControl(SETTING_USERNAME);
  if (userControl != NULL && userControl->GetControl() != NULL)
  {
    int userControlID = userControl->GetID();
    SET_CONTROL_LABEL2(userControlID, m_username);
    CONTROL_ENABLE_ON_CONDITION(userControlID, m_protocol != NET_PROTOCOL_UPNP &&
                                               m_protocol != NET_PROTOCOL_NFS);

    SendMessage(GUI_MSG_SET_TYPE, userControlID, CGUIEditControl::INPUT_TYPE_TEXT, 1019);
  }

  // port
  BaseSettingControlPtr portControl = GetSettingControl(SETTING_PORT_NUMBER);
  if (portControl != NULL && portControl->GetControl() != NULL)
  {
    int portControlID = portControl->GetID();
    SET_CONTROL_LABEL2(portControlID, m_port);
    CONTROL_ENABLE_ON_CONDITION(portControlID, m_protocol == NET_PROTOCOL_FTP ||
                                               m_protocol == NET_PROTOCOL_HTTP ||
                                               m_protocol == NET_PROTOCOL_HTTPS ||
                                               m_protocol == NET_PROTOCOL_DAV ||
                                               m_protocol == NET_PROTOCOL_DAVS ||
                                               m_protocol == NET_PROTOCOL_RSS ||
                                               m_protocol == NET_PROTOCOL_SFTP);

    SendMessage(GUI_MSG_SET_TYPE, portControlID, CGUIEditControl::INPUT_TYPE_NUMBER, 1018);
  }

  // password
  BaseSettingControlPtr passControl = GetSettingControl(SETTING_PASSWORD);
  if (passControl != NULL && passControl->GetControl() != NULL)
  {
    int passControlID = passControl->GetID();
    SET_CONTROL_LABEL2(passControlID, m_password);
    CONTROL_ENABLE_ON_CONDITION(passControlID, m_protocol != NET_PROTOCOL_UPNP &&
                                                  m_protocol != NET_PROTOCOL_NFS);

    SendMessage(GUI_MSG_SET_TYPE, passControlID, CGUIEditControl::INPUT_TYPE_PASSWORD, 12326);
  }

  // server browse should be disabled if we are in FTP, HTTP, HTTPS, RSS, DAV or DAVS
  BaseSettingControlPtr browseControl = GetSettingControl(SETTING_SERVER_BROWSE);
  if (browseControl != NULL && browseControl->GetControl() != NULL)
  {
    int browseControlID = browseControl->GetID();
    CONTROL_ENABLE_ON_CONDITION(browseControlID, !m_server.empty() || !(m_protocol == NET_PROTOCOL_FTP ||
                                                                        m_protocol == NET_PROTOCOL_HTTP ||
                                                                        m_protocol == NET_PROTOCOL_HTTPS ||
                                                                        m_protocol == NET_PROTOCOL_DAV ||
                                                                        m_protocol == NET_PROTOCOL_DAVS ||
                                                                        m_protocol == NET_PROTOCOL_RSS ||
                                                                        m_protocol == NET_PROTOCOL_SFTP));
  }
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

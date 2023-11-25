/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogNetworkSetup.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/VFSEntry.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <utility>


using namespace ADDON;
using namespace KODI::MESSAGING;


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
  m_protocol = 0;
  m_confirmed = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogNetworkSetup::~CGUIDialogNetworkSetup() = default;

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

void CGUIDialogNetworkSetup::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
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

void CGUIDialogNetworkSetup::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
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
  CGUIDialogNetworkSetup *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogNetworkSetup>(WINDOW_DIALOG_NETWORK_SETUP);
  if (!dialog) return false;
  dialog->Initialize();
  if (!dialog->SetPath(path))
  {
    HELPERS::ShowOKDialogText(CVariant{ 10218 }, CVariant{ 39103 });
    return false;
  }

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
  for (size_t idx = 0; idx < m_protocols.size(); ++idx)
    labels.emplace_back(m_protocols[idx].label, static_cast<int>(idx), m_protocols[idx].addonId);

  AddSpinner(group, SETTING_PROTOCOL, 1008, SettingLevel::Basic, m_protocol, labels);
  AddEdit(group, SETTING_SERVER_ADDRESS, 1010, SettingLevel::Basic, m_server, true);
  std::shared_ptr<CSettingAction> subsetting = AddButton(group, SETTING_SERVER_BROWSE, 1024, SettingLevel::Basic, "", false);
  if (subsetting != NULL)
    subsetting->SetParent(SETTING_SERVER_ADDRESS);

  AddEdit(group, SETTING_REMOTE_PATH, 1012, SettingLevel::Basic, m_path, true);
  AddEdit(group, SETTING_PORT_NUMBER, 1013, SettingLevel::Basic, m_port, true);
  AddEdit(group, SETTING_USERNAME, 1014, SettingLevel::Basic, m_username, true);
  AddEdit(group, SETTING_PASSWORD, 15052, SettingLevel::Basic, m_password, true, true);
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
    m_protocol = msg.GetParam1();
    // set defaults for the port
    m_port = std::to_string(m_protocols[m_protocol].defaultPort);

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
    if (m_protocols[m_protocol].type == "smb")
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
    CONTROL_ENABLE_ON_CONDITION(pathControlID, m_protocols[m_protocol].supportPath);
    if (m_protocols[m_protocol].type != "smb")
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
    CONTROL_ENABLE_ON_CONDITION(userControlID,
                                m_protocols[m_protocol].supportUsername);

    SendMessage(GUI_MSG_SET_TYPE, userControlID, CGUIEditControl::INPUT_TYPE_TEXT, 1019);
  }

  // port
  BaseSettingControlPtr portControl = GetSettingControl(SETTING_PORT_NUMBER);
  if (portControl != NULL && portControl->GetControl() != NULL)
  {
    int portControlID = portControl->GetID();
    SET_CONTROL_LABEL2(portControlID, m_port);
    CONTROL_ENABLE_ON_CONDITION(portControlID, m_protocols[m_protocol].supportPort);

    SendMessage(GUI_MSG_SET_TYPE, portControlID, CGUIEditControl::INPUT_TYPE_NUMBER, 1018);
  }

  // password
  BaseSettingControlPtr passControl = GetSettingControl(SETTING_PASSWORD);
  if (passControl != NULL && passControl->GetControl() != NULL)
  {
    int passControlID = passControl->GetID();
    SET_CONTROL_LABEL2(passControlID, m_password);
    CONTROL_ENABLE_ON_CONDITION(passControlID,
                                m_protocols[m_protocol].supportPassword);

    SendMessage(GUI_MSG_SET_TYPE, passControlID, CGUIEditControl::INPUT_TYPE_PASSWORD, 12326);
  }

  // server browse should be disabled if we are in FTP, FTPS, HTTP, HTTPS, RSS, RSSS, DAV or DAVS
  BaseSettingControlPtr browseControl = GetSettingControl(SETTING_SERVER_BROWSE);
  if (browseControl != NULL && browseControl->GetControl() != NULL)
  {
    int browseControlID = browseControl->GetID();
    CONTROL_ENABLE_ON_CONDITION(browseControlID,
                                m_protocols[m_protocol].supportBrowsing);
  }
}

std::string CGUIDialogNetworkSetup::ConstructPath() const
{
  CURL url;
  url.SetProtocol(m_protocols[m_protocol].type);

  if (!m_username.empty())
  {
    // domain/name to domain\name
    std::string username = m_username;
    std::replace(username.begin(), username.end(), '/', '\\');

    if (url.IsProtocol("smb") && username.find('\\') != std::string::npos)
    {
      auto pair = StringUtils::Split(username, "\\", 2);
      url.SetDomain(pair[0]);
      url.SetUserName(pair[1]);
    }
    else
      url.SetUserName(m_username);
    if (!m_password.empty())
      url.SetPassword(m_password);
  }

  if (!m_server.empty())
    url.SetHostName(m_server);

  if (m_protocols[m_protocol].supportPort &&
      !m_port.empty() && atoi(m_port.c_str()) > 0)
  {
    url.SetPort(atoi(m_port.c_str()));
  }

  if (!m_path.empty())
    url.SetFileName(m_path);

  return url.Get();
}

bool CGUIDialogNetworkSetup::SetPath(const std::string &path)
{
  UpdateAvailableProtocols();

  if (path.empty())
  {
    Reset();
    return true;
  }

  CURL url(path);
  m_protocol = -1;
  for (size_t i = 0; i < m_protocols.size(); ++i)
  {
    if (m_protocols[i].type == url.GetProtocol())
    {
      m_protocol = i;
      break;
    }
  }
  if (m_protocol == -1)
  {
    CLog::LogF(LOGERROR, "Asked to initialize for unknown path {}", path);
    Reset();
    return false;
  }

  if (!url.GetDomain().empty())
    m_username = url.GetDomain() + "\\" + url.GetUserName();
  else
    m_username = url.GetUserName();
  m_password = url.GetPassWord();
  m_port = std::to_string(url.GetPort());
  m_server = url.GetHostName();
  m_path = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(m_path);

  return true;
}

void CGUIDialogNetworkSetup::Reset()
{
  m_username.clear();
  m_password.clear();
  m_port.clear();
  m_server.clear();
  m_path.clear();
  m_protocol = 0;
}

void CGUIDialogNetworkSetup::UpdateAvailableProtocols()
{
  m_protocols.clear();
#ifdef HAS_FILESYSTEM_SMB
  // most popular protocol at the first place
  m_protocols.emplace_back(Protocol{true, true, true, false, true, 0, "smb", 20171, ""});
#endif
  // protocols from vfs addon next
  if (CServiceBroker::IsAddonInterfaceUp())
  {
    for (const auto& addon : CServiceBroker::GetVFSAddonCache().GetAddonInstances())
    {
      const auto& info = addon->GetProtocolInfo();
      if (!addon->GetProtocolInfo().type.empty())
      {
        // only use first protocol
        auto prots = StringUtils::Split(info.type, "|");
        m_protocols.emplace_back(Protocol{
            info.supportPath, info.supportUsername, info.supportPassword, info.supportPort,
            info.supportBrowsing, info.defaultPort, prots.front(), info.label, addon->ID()});
      }
    }
  }
  // internals
  const std::vector<Protocol> defaults = {{true, true, true, true, false, 443, "https", 20301, ""},
                                          {true, true, true, true, false, 80, "http", 20300, ""},
                                          {true, true, true, true, false, 443, "davs", 20254, ""},
                                          {true, true, true, true, false, 80, "dav", 20253, ""},
                                          {true, true, true, true, false, 21, "ftp", 20173, ""},
                                          {true, true, true, true, false, 990, "ftps", 20174, ""},
                                          {false, false, false, false, true, 0, "upnp", 20175, ""},
                                          {true, true, true, true, false, 80, "rss", 20304, ""},
                                          {true, true, true, true, false, 443, "rsss", 20305, ""}};

  m_protocols.insert(m_protocols.end(), defaults.begin(), defaults.end());
#ifdef HAS_FILESYSTEM_NFS
  m_protocols.emplace_back(Protocol{true, false, false, false, true, 0, "nfs", 20259, ""});
#endif
}

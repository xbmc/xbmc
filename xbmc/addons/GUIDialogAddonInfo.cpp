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

#include "GUIDialogAddonInfo.h"

#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "AddonDatabase.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "GUIDialogAddonSettings.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "pictures/GUIWindowSlideShow.h"
#include "settings/Settings.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "Util.h"
#include "interfaces/builtins/Builtins.h"

#include <functional>
#include <utility>

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_ENABLE           7
#define CONTROL_BTN_UPDATE           8
#define CONTROL_BTN_SETTINGS         9
#define CONTROL_BTN_SELECT          12
#define CONTROL_BTN_AUTOUPDATE      13
#define CONTROL_LIST_SCREENSHOTS    50

using namespace ADDON;
using namespace XFILE;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
  : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml"),
  m_addonEnabled(false)
{
  m_item = CFileItemPtr(new CFileItem);
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogAddonInfo::~CGUIDialogAddonInfo(void)
{
}

bool CGUIDialogAddonInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_UPDATE)
      {
        OnUpdate();
        return true;
      }
      if (iControl == CONTROL_BTN_INSTALL)
      {
        if (m_localAddon)
        {
          if (m_localAddon->Type() == ADDON_ADSPDLL && CServiceBroker::GetADSP().IsProcessing())
          {
            CGUIDialogOK::ShowAndGetInput(24137, 0, 24138, 0);
            return true;
          }
        }

        if (!m_localAddon)
        {
          OnInstall();
          return true;
        }
        else
        {
          OnUninstall();
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_SELECT)
      {
        OnSelect();
        return true;
      }
      else if (iControl == CONTROL_BTN_ENABLE)
      {
        //FIXME: should be moved to somewhere appropriate (e.g CAddonMgs::CanAddonBeDisabled or IsInUse) and button should be disabled
        if (m_localAddon)
        {
          if (m_localAddon->Type() == ADDON_ADSPDLL && CServiceBroker::GetADSP().IsProcessing())
          {
            CGUIDialogOK::ShowAndGetInput(24137, 0, 24138, 0);
            return true;
          }
        }

        OnEnableDisable();
        return true;
      }
      else if (iControl == CONTROL_BTN_SETTINGS)
      {
        OnSettings();
        return true;
      }
      else if (iControl == CONTROL_BTN_AUTOUPDATE)
      {
        OnToggleAutoUpdates();
        return true;
      }
      else if (iControl == CONTROL_LIST_SCREENSHOTS)
      {
        if (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          OnMessage(msg);
          int start = msg.GetParam1();
          if (start >= 0 && start < m_item->GetAddonInfo()->Screenshots().size())
            CGUIWindowSlideShow::RunSlideShow(m_item->GetAddonInfo()->Screenshots(), start);
        }
      }
    }
    break;
default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogAddonInfo::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogAddonInfo::OnInitWindow()
{
  UpdateControls();
  CGUIDialog::OnInitWindow();
}

void CGUIDialogAddonInfo::UpdateControls()
{
  if (!m_item)
    return;

  bool isInstalled = NULL != m_localAddon.get();
  m_addonEnabled = m_localAddon && !CAddonMgr::GetInstance().IsAddonDisabled(m_localAddon->ID());
  bool canDisable = isInstalled && CAddonMgr::GetInstance().CanAddonBeDisabled(m_localAddon->ID());
  bool canInstall = !isInstalled && m_item->GetAddonInfo()->Broken().empty();
  bool canUninstall = m_localAddon && CAddonMgr::GetInstance().CanUninstall(m_localAddon);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canInstall || canUninstall);
  SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, isInstalled ? 24037 : 24038);

  if (m_addonEnabled)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 24021);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, canDisable);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 24022);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, isInstalled);
  }

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, isInstalled);

  bool autoUpdatesOn = CSettings::GetInstance().GetInt(CSettings::SETTING_ADDONS_AUTOUPDATES) == AUTO_UPDATES_ON;
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn &&
      !CAddonMgr::GetInstance().IsBlacklisted(m_localAddon->ID()));
  SET_CONTROL_LABEL(CONTROL_BTN_AUTOUPDATE, 21340);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SELECT, m_addonEnabled && (CanOpen() ||
      CanRun() || (CanUse() && !m_localAddon->IsInUse())));
  SET_CONTROL_LABEL(CONTROL_BTN_SELECT, CanUse() ? 21480 : (CanOpen() ? 21478 : 21479));

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && m_localAddon->HasSettings());

  CFileItemList items;
  for (const auto& screenshot : m_item->GetAddonInfo()->Screenshots())
  {
    auto item = std::make_shared<CFileItem>("");
    item->SetArt("thumb", screenshot);
    items.Add(std::move(item));
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_SCREENSHOTS, 0, 0, &items);
  OnMessage(msg);
}

static const std::string LOCAL_CACHE = "\\0_local_cache"; // \0 to give it the lowest priority when sorting


int CGUIDialogAddonInfo::AskForVersion(std::vector<std::pair<AddonVersion, std::string>>& versions)
{
  auto dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
  dialog->Reset();
  dialog->SetHeading(CVariant{21338});
  dialog->SetUseDetails(true);

  std::sort(versions.begin(), versions.end(), std::greater<std::pair<AddonVersion, std::string>>());

  for (const auto& versionInfo : versions)
  {
    CFileItem item(StringUtils::Format(g_localizeStrings.Get(21339).c_str(), versionInfo.first.asString().c_str()));
    if (m_localAddon && m_localAddon->Version() == versionInfo.first
        && m_item->GetAddonInfo()->Origin() == versionInfo.second)
      item.Select(true);

    AddonPtr repo;
    if (versionInfo.second == LOCAL_CACHE)
    {
      item.SetLabel2(g_localizeStrings.Get(24095));
      item.SetIconImage("DefaultAddonRepository.png");
      dialog->Add(item);
    }
    else if (CAddonMgr::GetInstance().GetAddon(versionInfo.second, repo, ADDON_REPOSITORY))
    {
      item.SetLabel2(repo->Name());
      item.SetIconImage(repo->Icon());
      dialog->Add(item);
    }
  }

  dialog->Open();
  return dialog->IsConfirmed() ? dialog->GetSelectedItem() : -1;
}

void CGUIDialogAddonInfo::OnUpdate()
{
  if (!m_localAddon)
    return;

  std::vector<std::pair<AddonVersion, std::string>> versions;

  CAddonDatabase database;
  database.Open();
  database.GetAvailableVersions(m_localAddon->ID(), versions);

  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory("special://home/addons/packages/", items, ".zip", DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string packageId;
      std::string versionString;
      if (AddonVersion::SplitFileName(packageId, versionString, items[i]->GetLabel()))
      {
        if (packageId == m_localAddon->ID())
        {
          std::string hash;
          std::string path(items[i]->GetPath());
          if (database.GetPackageHash(m_localAddon->ID(), items[i]->GetPath(), hash))
          {
            std::string md5 = CUtil::GetFileMD5(path);
            if (md5 == hash)
              versions.push_back(std::make_pair(AddonVersion(versionString), LOCAL_CACHE));
          }
        }
      }
    }
  }

  if (versions.empty())
    CGUIDialogOK::ShowAndGetInput(CVariant{21341}, CVariant{21342});
  else
  {
    int i = AskForVersion(versions);
    if (i != -1)
    {
      Close();
      //turn auto updating off if downgrading
      if (m_localAddon->Version() > versions[i].first)
        CAddonMgr::GetInstance().AddToUpdateBlacklist(m_localAddon->ID());

      if (versions[i].second == LOCAL_CACHE)
        CAddonInstaller::GetInstance().InstallFromZip(StringUtils::Format(
            "special://home/addons/packages/%s-%s.zip", m_localAddon->ID().c_str(),
            versions[i].first.asString().c_str()));
      else
        CAddonInstaller::GetInstance().Install(m_localAddon->ID(), versions[i].first, versions[i].second);
    }
  }
}

void CGUIDialogAddonInfo::OnToggleAutoUpdates()
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), CONTROL_BTN_AUTOUPDATE);
  if (OnMessage(msg))
  {
    bool selected = msg.GetParam1() == 1;
    if (selected)
      CAddonMgr::GetInstance().RemoveFromUpdateBlacklist(m_localAddon->ID());
    else
      CAddonMgr::GetInstance().AddToUpdateBlacklist(m_localAddon->ID());
  }
}

void CGUIDialogAddonInfo::OnInstall()
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  if (m_localAddon || !m_item->HasAddonInfo())
    return;

  std::string addonId = m_item->GetAddonInfo()->ID();
  std::vector<std::pair<AddonVersion, std::string>> versions;

  CAddonDatabase database;
  if (!database.Open() || !database.GetAvailableVersions(addonId, versions) || versions.empty())
  {
    CLog::Log(LOGERROR, "ADDON: no available versions of %s", addonId.c_str());
    return;
  }

  int i = versions.size() == 1 ? 0 : AskForVersion(versions);
  if (i != -1)
  {
    Close();
    CAddonInstaller::GetInstance().Install(addonId, versions[i].first, versions[i].second);
  }
}

void CGUIDialogAddonInfo::OnSelect()
{
  if (!m_localAddon)
    return;

  Close();

  if (CanOpen() || CanRun())
    CBuiltins::GetInstance().Execute("RunAddon(" + m_localAddon->ID() + ")");
  else if (CanUse())
    CAddonMgr::GetInstance().SetDefault(m_localAddon->Type(), m_localAddon->ID());
}

bool CGUIDialogAddonInfo::CanOpen() const
{
  return m_localAddon && m_localAddon->Type() == ADDON_PLUGIN;
}

bool CGUIDialogAddonInfo::CanRun() const
{
  return m_localAddon && m_localAddon->Type() == ADDON_SCRIPT;
}

bool CGUIDialogAddonInfo::CanUse() const
{
  return m_localAddon && (
    m_localAddon->Type() == ADDON_SKIN ||
    m_localAddon->Type() == ADDON_SCREENSAVER ||
    m_localAddon->Type() == ADDON_VIZ ||
    m_localAddon->Type() == ADDON_SCRIPT_WEATHER ||
    m_localAddon->Type() == ADDON_RESOURCE_LANGUAGE ||
    m_localAddon->Type() == ADDON_RESOURCE_UISOUNDS);
}

bool CGUIDialogAddonInfo::PromptIfDependency(int heading, int line2)
{
  if (!m_localAddon)
    return false;

  VECADDONS addons;
  std::vector<std::string> deps;
  CAddonMgr::GetInstance().GetAddons(addons);
  for (VECADDONS::const_iterator it  = addons.begin();
       it != addons.end();++it)
  {
    ADDONDEPS::const_iterator i = (*it)->GetDeps().find(m_localAddon->ID());
    if (i != (*it)->GetDeps().end() && !i->second.second) // non-optional dependency
      deps.push_back((*it)->Name());
  }

  if (!deps.empty())
  {
    std::string line0 = StringUtils::Format(g_localizeStrings.Get(24046).c_str(), m_localAddon->Name().c_str());
    std::string line1 = StringUtils::Join(deps, ", ");
    CGUIDialogOK::ShowAndGetInput(CVariant{heading}, CVariant{std::move(line0)}, CVariant{std::move(line1)}, CVariant{line2});
    return true;
  }
  return false;
}

void CGUIDialogAddonInfo::OnUninstall()
{
  if (!m_localAddon.get())
    return;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  // ensure the addon is not a dependency of other installed addons
  if (PromptIfDependency(24037, 24047))
    return;

  // prompt user to be sure
  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{24037}, CVariant{750}))
    return;

  CJobManager::GetInstance().AddJob(new CAddonUnInstallJob(m_localAddon),
                                    &CAddonInstaller::GetInstance());
  Close();
}

void CGUIDialogAddonInfo::OnEnableDisable()
{
  if (!m_localAddon)
    return;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  if (m_addonEnabled)
  {
    if (PromptIfDependency(24075, 24091))
      return; //required. can't disable

    CAddonMgr::GetInstance().DisableAddon(m_localAddon->ID());
  }
  else
    CAddonMgr::GetInstance().EnableAddon(m_localAddon->ID());

  UpdateControls();
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowAndGetInput(m_localAddon);
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
  if (!item)
    return false;

  CGUIDialogAddonInfo* dialog = (CGUIDialogAddonInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_INFO);
  if (!dialog)
    return false;
  if (!dialog->SetItem(item))
    return false;

  dialog->Open(); 
  return true;
}

bool CGUIDialogAddonInfo::SetItem(const CFileItemPtr& item)
{
  if (!item || !item->HasAddonInfo())
    return false;

  m_item = item;
  m_localAddon.reset();
  CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->ID(), m_localAddon, ADDON_UNKNOWN, false);
  return true;
}

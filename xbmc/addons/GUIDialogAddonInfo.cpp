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
#include "AddonDatabase.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "GUIDialogAddonSettings.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "settings/Settings.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "Util.h"
#include "interfaces/builtins/Builtins.h"

#include <utility>

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_ENABLE           7
#define CONTROL_BTN_UPDATE           8
#define CONTROL_BTN_SETTINGS         9
#define CONTROL_BTN_CHANGELOG       10
#define CONTROL_BTN_SELECT          12
#define CONTROL_BTN_AUTOUPDATE      13

using namespace ADDON;
using namespace XFILE;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
  : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml"),
  m_jobid(0),
  m_changelog(false)
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
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_jobid)
        CJobManager::GetInstance().CancelJob(m_jobid);
    }
    break;

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
          if (m_localAddon->Type() == ADDON_ADSPDLL && ActiveAE::CActiveAEDSP::GetInstance().IsProcessing())
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
        if (m_localAddon)
        {
          if (m_localAddon->Type() == ADDON_ADSPDLL && ActiveAE::CActiveAEDSP::GetInstance().IsProcessing())
          {
            CGUIDialogOK::ShowAndGetInput(24137, 0, 24138, 0);
            return true;
          }
        }

        OnEnable(!m_item->GetProperty("Addon.Enabled").asBoolean());
        return true;
      }
      else if (iControl == CONTROL_BTN_SETTINGS)
      {
        OnSettings();
        return true;
      }
      else if (iControl == CONTROL_BTN_CHANGELOG)
      {
        OnChangeLog();
        return true;
      }
      else if (iControl == CONTROL_BTN_AUTOUPDATE)
      {
        OnToggleAutoUpdates();
        return true;
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
  m_changelog = false;
}

void CGUIDialogAddonInfo::UpdateControls()
{
  bool isInstalled = NULL != m_localAddon.get();
  bool isEnabled = isInstalled && m_item->GetProperty("Addon.Enabled").asBoolean();
  bool canDisable = isInstalled && CAddonMgr::GetInstance().CanAddonBeDisabled(m_localAddon->ID());
  bool canInstall = !isInstalled && m_item->GetProperty("Addon.Broken").empty();
  bool isRepo = (isInstalled && m_localAddon->Type() == ADDON_REPOSITORY) || (m_addon && m_addon->Type() == ADDON_REPOSITORY);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canDisable || canInstall);
  SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, isInstalled ? 24037 : 24038);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, canDisable);
  SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, isEnabled ? 24021 : 24022);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, isInstalled);

  bool autoUpdatesOn = CSettings::GetInstance().GetInt(CSettings::SETTING_GENERAL_ADDONUPDATES) == AUTO_UPDATES_ON;
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn &&
      !CAddonMgr::GetInstance().IsBlacklisted(m_localAddon->ID()));
  SET_CONTROL_LABEL(CONTROL_BTN_AUTOUPDATE, 21340);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SELECT, isEnabled && (CanOpen() ||
      CanRun() || (CanUse() && !m_localAddon->IsInUse())));
  SET_CONTROL_LABEL(CONTROL_BTN_SELECT, CanUse() ? 21480 : (CanOpen() ? 21478 : 21479));

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && m_localAddon->HasSettings());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_CHANGELOG, !isRepo);
}

static const std::string LOCAL_CACHE = "special_local_cache";

static bool CompareVersion(const std::pair<AddonVersion, std::string>& lhs, const std::pair<AddonVersion, std::string>& rhs)
{
  return lhs.first > rhs.first;
};

void CGUIDialogAddonInfo::OnUpdate()
{
  if (!m_localAddon)
    return;

  CAddonDatabase database;
  if (!database.Open())
    return;

  std::vector<std::pair<AddonVersion, std::string>> versions;
  if (!database.GetAvailableVersions(m_localAddon->ID(), versions))
    return;

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
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{21341}, CVariant{21342});
    return;
  }

  auto* dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
  dialog->Reset();
  dialog->SetHeading(CVariant{21338});
  dialog->SetUseDetails(true);

  std::stable_sort(versions.begin(), versions.end(), CompareVersion);

  for (const auto& versionInfo : versions)
  {
    CFileItem item(StringUtils::Format(g_localizeStrings.Get(21339).c_str(), versionInfo.first.asString().c_str()));
    if (versionInfo.first == m_localAddon->Version())
      item.Select(true);

    AddonPtr repo;
    if (versionInfo.second == LOCAL_CACHE)
    {
      item.SetProperty("Addon.Summary", g_localizeStrings.Get(24095));
      item.SetIconImage("DefaultAddonRepository.png");
      dialog->Add(item);
    }
    else if (CAddonMgr::GetInstance().GetAddon(versionInfo.second, repo, ADDON_REPOSITORY))
    {
      item.SetProperty("Addon.Summary", repo->Name());
      item.SetIconImage(repo->Icon());
      dialog->Add(item);
    }
  }

  dialog->Open();
  if (dialog->IsConfirmed())
  {
    Close();

    auto selected = versions.at(dialog->GetSelectedLabel());

    //turn auto updating off if downgrading
    if (selected.first < m_localAddon->Version())
      CAddonMgr::GetInstance().AddToUpdateBlacklist(m_localAddon->ID());

    if (selected.second == LOCAL_CACHE)
      CAddonInstaller::GetInstance().InstallFromZip(StringUtils::Format("special://home/addons/packages/%s-%s.zip",
          m_localAddon->ID().c_str(), selected.first.asString().c_str()));
    else
      CAddonInstaller::GetInstance().Install(m_addon->ID(), selected.first, selected.second);
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

  if (!m_addon)
    return;

  CAddonInstaller::GetInstance().InstallOrUpdate(m_addon->ID());
  Close();
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
  CAddonMgr::GetInstance().GetAllAddons(addons);
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

void CGUIDialogAddonInfo::OnEnable(bool enable)
{
  if (!m_localAddon.get())
    return;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  if (!enable && PromptIfDependency(24075, 24091))
    return;

  if (enable)
    CAddonMgr::GetInstance().EnableAddon(m_localAddon->ID());
  else
    CAddonMgr::GetInstance().DisableAddon(m_localAddon->ID());
  SetItem(m_item);
  UpdateControls();
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowAndGetInput(m_localAddon);
}

void CGUIDialogAddonInfo::OnChangeLog()
{
  CGUIDialogTextViewer* pDlgInfo = (CGUIDialogTextViewer*)g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);
  std::string name;
  if (m_addon)
    name = m_addon->Name();
  else if (m_localAddon)
    name = m_localAddon->Name();
  pDlgInfo->SetHeading(g_localizeStrings.Get(24054)+" - "+name);
  if (m_item->GetProperty("Addon.Changelog").empty())
  {
    pDlgInfo->SetText(g_localizeStrings.Get(13413));
    CFileItemList items;
    if (m_localAddon && 
        !m_item->GetProperty("Addon.UpdateAvail").asBoolean())
    {
      items.Add(CFileItemPtr(new CFileItem(m_localAddon->ChangeLog(),false)));
    }
    else
      items.Add(CFileItemPtr(new CFileItem(m_addon->ChangeLog(),false)));
    items[0]->Select(true);
    m_jobid = CJobManager::GetInstance().AddJob(
      new CFileOperationJob(CFileOperationJob::ActionCopy,items,
                            "special://temp/"),this);
  }
  else
    pDlgInfo->SetText(m_item->GetProperty("Addon.Changelog").asString());

  m_changelog = true;
  pDlgInfo->Open();
  m_changelog = false;
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
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
  *m_item = *item;

  // grab the local addon, if it's available
  m_localAddon.reset();
  m_addon.reset();
  if (CAddonMgr::GetInstance().GetAddon(item->GetProperty("Addon.ID").asString(), m_localAddon)) // sets m_localAddon if installed regardless of enabled state
    m_item->SetProperty("Addon.Enabled", "true");
  else
    m_item->SetProperty("Addon.Enabled", "false");
  m_item->SetProperty("Addon.Installed", m_localAddon ? "true" : "false");

  CAddonDatabase database;
  database.Open();
  database.GetAddon(item->GetProperty("Addon.ID").asString(),m_addon);

  if (TranslateType(item->GetProperty("Addon.intType").asString()) == ADDON_REPOSITORY)
  {
    CAddonDatabase database;
    database.Open();
    VECADDONS addons;
    if (m_addon)
      database.GetRepositoryContent(m_addon->ID(), addons);
    else if (m_localAddon) // sanity
      database.GetRepositoryContent(m_localAddon->ID(), addons);
    int tot=0;
    for (int i = ADDON_UNKNOWN+1;i<ADDON_MAX;++i)
    {
      int num=0;
      for (unsigned int j=0;j<addons.size();++j)
      {
        if (addons[j]->Type() == (TYPE)i)
          ++num;
      }
      m_item->SetProperty("Repo." + TranslateType((TYPE)i), num);
      tot += num;
    }
    m_item->SetProperty("Repo.Addons", tot);
  }
  return true;
}

void CGUIDialogAddonInfo::OnJobComplete(unsigned int jobID, bool success,
                                        CJob* job)
{
  if (!m_changelog)
    return;

  CGUIDialogTextViewer* pDlgInfo = (CGUIDialogTextViewer*)g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);

  m_jobid = 0;
  if (!success)
  {
    pDlgInfo->SetText(g_localizeStrings.Get(195));
  }
  else
  {
    CFile file;
    XFILE::auto_buffer buf;
    if (file.LoadFile("special://temp/" +
      URIUtils::GetFileName(((CFileOperationJob*)job)->GetItems()[0]->GetPath()), buf) > 0)
    {
      std::string str(buf.get(), buf.length());
      m_item->SetProperty("Addon.Changelog", str);
      pDlgInfo->SetText(str);
    }
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_DIALOG_TEXT_VIEWER, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

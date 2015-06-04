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
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "addons/AddonManager.h"
#include "AddonDatabase.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "addons/AddonInstaller.h"
#include "Util.h"
#include "interfaces/Builtins.h"

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_ENABLE           7
#define CONTROL_BTN_UPDATE           8
#define CONTROL_BTN_SETTINGS         9
#define CONTROL_BTN_CHANGELOG       10
#define CONTROL_BTN_ROLLBACK        11
#define CONTROL_BTN_SELECT          12

using namespace std;
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
        OnLaunch();
        return true;
      }
      else if (iControl == CONTROL_BTN_ENABLE)
      {
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
      else if (iControl == CONTROL_BTN_ROLLBACK)
      {
        OnRollback();
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
  bool isUpdatable = isInstalled && m_item->GetProperty("Addon.UpdateAvail").asBoolean();
  bool isExecutable = isInstalled && (m_localAddon->Type() == ADDON_PLUGIN || m_localAddon->Type() == ADDON_SCRIPT);
  if (isInstalled)
    GrabRollbackVersions();

  bool canDisable = isInstalled && CAddonMgr::Get().CanAddonBeDisabled(m_localAddon->ID());
  bool canInstall = !isInstalled && m_item->GetProperty("Addon.Broken").empty();
  bool isRepo = (isInstalled && m_localAddon->Type() == ADDON_REPOSITORY) || (m_addon && m_addon->Type() == ADDON_REPOSITORY);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canDisable || canInstall);
  SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, isInstalled ? 24037 : 24038);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, canDisable);
  SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, isEnabled ? 24021 : 24022);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, isUpdatable);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && m_localAddon->HasSettings());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SELECT, isExecutable);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_CHANGELOG, !isRepo);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ROLLBACK, m_rollbackVersions.size() > 1);
}

void CGUIDialogAddonInfo::OnUpdate()
{
  std::string referer = StringUtils::Format("Referer=%s-%s.zip",m_localAddon->ID().c_str(),m_localAddon->Version().asString().c_str());
  CAddonInstaller::Get().Install(m_addon->ID(), true, referer); // force install
  Close();
}

void CGUIDialogAddonInfo::OnInstall()
{
  CAddonInstaller::Get().Install(m_addon->ID());
  Close();
}

void CGUIDialogAddonInfo::OnLaunch()
{
  if (!m_localAddon)
    return;

  Close();
  CBuiltins::Execute("RunAddon(" + m_localAddon->ID() + ")");
}

bool CGUIDialogAddonInfo::PromptIfDependency(int heading, int line2)
{
  if (!m_localAddon)
    return false;

  VECADDONS addons;
  vector<string> deps;
  CAddonMgr::Get().GetAllAddons(addons);
  for (VECADDONS::const_iterator it  = addons.begin();
       it != addons.end();++it)
  {
    ADDONDEPS::const_iterator i = (*it)->GetDeps().find(m_localAddon->ID());
    if (i != (*it)->GetDeps().end() && !i->second.second) // non-optional dependency
      deps.push_back((*it)->Name());
  }

  if (!deps.empty())
  {
    string line0 = StringUtils::Format(g_localizeStrings.Get(24046).c_str(), m_localAddon->Name().c_str());
    string line1 = StringUtils::Join(deps, ", ");
    CGUIDialogOK::ShowAndGetInput(heading, line0, line1, line2);
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
  if (!CGUIDialogYesNo::ShowAndGetInput(24037, 750))
    return;

  CJobManager::GetInstance().AddJob(new CAddonUnInstallJob(m_localAddon),
                                    &CAddonInstaller::Get());
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
    CAddonMgr::Get().EnableAddon(m_localAddon->ID());
  else
    CAddonMgr::Get().DisableAddon(m_localAddon->ID());
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
  pDlgInfo->DoModal();
  m_changelog = false;
}

void CGUIDialogAddonInfo::OnRollback()
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  CGUIDialogContextMenu* dlg = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
  CAddonDatabase database;
  database.Open();

  CContextButtons buttons;
  for (unsigned int i=0;i<m_rollbackVersions.size();++i)
  {
    std::string label(m_rollbackVersions[i]);
    if (m_rollbackVersions[i] == m_localAddon->Version().asString())
     label += " "+g_localizeStrings.Get(24094);
   if (database.IsAddonBlacklisted(m_localAddon->ID(),label))
     label += " "+g_localizeStrings.Get(24095);

    buttons.Add(i,label);
  }
  int choice;
  if ((choice=dlg->ShowAndGetChoice(buttons)) > -1)
  {
    // blacklist everything newer
    for (unsigned int j=choice+1;j<m_rollbackVersions.size();++j)
      database.BlacklistAddon(m_localAddon->ID(),m_rollbackVersions[j]);
    std::string path = "special://home/addons/packages/";
    path += m_localAddon->ID()+"-"+m_rollbackVersions[choice]+".zip";

    //FIXME: this is probably broken
    // needed as cpluff won't downgrade
    if (!m_localAddon->IsType(ADDON_SERVICE))
      //we will handle this for service addons in CAddonInstallJob::OnPostInstall
      CAddonMgr::Get().UnregisterAddon(m_localAddon->ID());
    CAddonInstaller::Get().InstallFromZip(path);
    database.RemoveAddonFromBlacklist(m_localAddon->ID(),m_rollbackVersions[choice]);
    Close();
  }
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
  CGUIDialogAddonInfo* dialog = (CGUIDialogAddonInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_INFO);
  if (!dialog)
    return false;
  if (!dialog->SetItem(item))
    return false;

  dialog->DoModal(); 
  return true;
}

bool CGUIDialogAddonInfo::SetItem(const CFileItemPtr& item)
{
  *m_item = *item;
  m_rollbackVersions.clear();

  // grab the local addon, if it's available
  m_localAddon.reset();
  m_addon.reset();
  if (CAddonMgr::Get().GetAddon(item->GetProperty("Addon.ID").asString(), m_localAddon)) // sets m_localAddon if installed regardless of enabled state
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
      database.GetRepository(m_addon->ID(), addons);
    else if (m_localAddon) // sanity
      database.GetRepository(m_localAddon->ID(), addons);
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

void CGUIDialogAddonInfo::GrabRollbackVersions()
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory("special://home/addons/packages/",items,".zip",DIR_FLAG_NO_FILE_DIRS);
  items.Sort(SortByLabel, SortOrderAscending);
  CAddonDatabase db;
  db.Open();
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    std::string ID, version;
    AddonVersion::SplitFileName(ID,version,items[i]->GetLabel());
    if (ID == m_localAddon->ID())
    {
      std::string hash, path(items[i]->GetPath());
      if (db.GetPackageHash(m_localAddon->ID(), path, hash))
      {
        std::string md5 = CUtil::GetFileMD5(path);
        if (md5 == hash)
          m_rollbackVersions.push_back(version);
        else /* The package has been corrupted */
        {
          CLog::Log(LOGWARNING, "%s: Removing corrupt addon package %s.", __FUNCTION__, path.c_str());
          CFile::Delete(path);
          db.RemovePackage(path);
        }
      }
    }
  }
}

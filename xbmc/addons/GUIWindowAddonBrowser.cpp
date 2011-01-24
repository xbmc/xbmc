/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIWindowAddonBrowser.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "filesystem/AddonsDirectory.h"
#include "utils/FileOperationJob.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "Application.h"
#include "AddonDatabase.h"
#include "settings/AdvancedSettings.h"
#include "storage/MediaManager.h"
#include "settings/GUISettings.h"

#define CONTROL_AUTOUPDATE 5
#define CONTROL_SHUTUP     6

using namespace ADDON;
using namespace XFILE;
using namespace std;


struct find_map : public binary_function<CGUIWindowAddonBrowser::JobMap::value_type, unsigned int, bool>
{
  bool operator() (CGUIWindowAddonBrowser::JobMap::value_type t, unsigned int id) const
  {
    return (t.second.jobID == id);
  }
};


CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIMediaWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
  m_thumbLoader.SetNumOfWorkers(1);
  m_promptReload = false;
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(false);

      // is this the first time the window is opened?
      if (m_vecItems->m_strPath == "?" && message.GetStringParam().IsEmpty())
        m_vecItems->m_strPath = "";
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_AUTOUPDATE)
      {
        g_settings.m_bAddonAutoUpdate = !g_settings.m_bAddonAutoUpdate;
        g_settings.Save();
        return true;
      }
      else if (iControl == CONTROL_SHUTUP)
      {
        g_settings.m_bAddonNotifications = !g_settings.m_bAddonNotifications;
        g_settings.Save();
        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_SHOW_INFO)
        {
          if (!m_vecItems->Get(iItem)->GetProperty("Addon.ID").IsEmpty())
            return CGUIDialogAddonInfo::ShowForItem((*m_vecItems)[iItem]);
          return false;
        }
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && IsActive() && message.GetNumStringParams() == 1)
      { // update this item
        for (int i = 0; i < m_vecItems->Size(); ++i)
        {
          CFileItemPtr item = m_vecItems->Get(i);
          if (item->GetProperty("Addon.ID") == message.GetStringParam())
          {
            SetItemLabel2(item);
            return true;
          }
        }
      }
    }
    break;
   default:
     break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowAddonBrowser::GetContextButtons(int itemNumber,
                                               CContextButtons& buttons)
{
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  if (pItem->m_strPath.Equals("addons://enabled/"))
    buttons.Add(CONTEXT_BUTTON_SCAN,24034);
  
  AddonPtr addon;
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID"), addon, ADDON_UNKNOWN, false)) // allow disabled addons
    return;

  if (addon->Type() == ADDON_REPOSITORY && pItem->m_bIsFolder)
  {
    buttons.Add(CONTEXT_BUTTON_SCAN,24034);
    buttons.Add(CONTEXT_BUTTON_UPDATE_LIBRARY,24035);
  }

  buttons.Add(CONTEXT_BUTTON_INFO,24003);

  if (addon->HasSettings())
    buttons.Add(CONTEXT_BUTTON_SETTINGS,24020);
}

bool CGUIWindowAddonBrowser::OnContextButton(int itemNumber,
                                             CONTEXT_BUTTON button)
{
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  if (pItem->m_strPath.Equals("addons://enabled/"))
  {
    if (button == CONTEXT_BUTTON_SCAN)
    {
      CAddonMgr::Get().FindAddons();
      return true;
    }
  }
  AddonPtr addon;
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID"), addon, ADDON_UNKNOWN, false)) // allow disabled addons
    return false;

  if (button == CONTEXT_BUTTON_SETTINGS)
    return CGUIDialogAddonSettings::ShowAndGetInput(addon);

  if (button == CONTEXT_BUTTON_UPDATE_LIBRARY)
  {
    CAddonDatabase database;
    database.Open();
    database.DeleteRepository(addon->ID());
    button = CONTEXT_BUTTON_SCAN;
  }

  if (button == CONTEXT_BUTTON_SCAN)
  {
    RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(addon);
    CJobManager::GetInstance().AddJob(new CRepositoryUpdateJob(repo,false),this);
    return true;
  }

  if (button == CONTEXT_BUTTON_INFO)
  {
    CGUIDialogAddonInfo::ShowForItem(pItem);
    return true;
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowAddonBrowser::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item->m_strPath == "addons://install/")
  {
    // pop up filebrowser to grab an installed folder
    VECSOURCES shares = g_settings.m_fileSources;
    g_mediaManager.GetLocalDrives(shares);
    CStdString path;
    if (CGUIDialogFileBrowser::ShowAndGetFile(shares, "*.zip", g_localizeStrings.Get(24041), path))
    {
      // install this addon
      AddJob(path);
    }
    return true;
  }
  if (!item->m_bIsFolder)
  {
    // cancel a downloading job
    if (item->HasProperty("Addon.Downloading"))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24000),
                                           item->GetProperty("Addon.Name"),
                                           g_localizeStrings.Get(24066),""))
      {
        CSingleLock lock(m_critSection);
        JobMap::iterator it = m_downloadJobs.find(item->GetProperty("Addon.ID"));
        if (it != m_downloadJobs.end())
        {
          CJobManager::GetInstance().CancelJob(it->second.jobID);
          m_downloadJobs.erase(it);
          Update(m_vecItems->m_strPath);
        }
      }
      return true;
    }

    CGUIDialogAddonInfo::ShowForItem(item);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowAddonBrowser::ReportInstallError(const CStdString& addonID,
                                                const CStdString& fileName)
{
  AddonPtr addon;
  CAddonDatabase database;
  database.Open();
  database.GetAddon(addonID, addon);
  if (addon)
  {
    AddonPtr addon2;
    CAddonMgr::Get().GetAddon(addonID, addon2);
    g_application.m_guiDialogKaiToast.QueueNotification(
                                          addon->Icon(),
                                          addon->Name(),
                                          g_localizeStrings.Get(addon2 ? 113 : 114),
                                          TOAST_DISPLAY_TIME, false);
  }
  else
  {
    g_application.m_guiDialogKaiToast.QueueNotification(
                                          CGUIDialogKaiToast::Error,
                                          fileName,
                                          g_localizeStrings.Get(114),
                                          TOAST_DISPLAY_TIME, false);
  }
}

void CGUIWindowAddonBrowser::ReportInstallErrorZip(const CStdString& zipName)
{
  CStdStringArray arr;
  // FIXME: this doesn't work if addon id contains dashes
  StringUtils::SplitString(zipName, "-", arr);
  ReportInstallError(arr[0], zipName);
}

bool CGUIWindowAddonBrowser::CheckHash(const CStdString& zipFile,
                                       const CStdString& hash)
{
  if (hash.IsEmpty())
    return true;
  CStdString package = URIUtils::AddFileToFolder("special://home/addons/packages/", zipFile);
  CStdString md5 = CUtil::GetFileMD5(package);
  if (!md5.Equals(hash))
  {
    CFile::Delete(package);
    ReportInstallErrorZip(zipFile);
    CLog::Log(LOGERROR,"MD5 mismatch after download %s", zipFile.c_str());
    return false;
  }

  return true;
}

void CGUIWindowAddonBrowser::OnJobComplete(unsigned int jobID,
                                           bool success, CJob* job2)
{
  if (success)
  {
    CFileOperationJob* job = (CFileOperationJob*)job2;
    if (job->GetAction() == CFileOperationJob::ActionReplace)
    {
      for (int i=0;i<job->GetItems().Size();++i)
      {
        CStdString strFolder = job->GetItems()[i]->m_strPath;
        // zip is downloaded - now extract it
        if (URIUtils::IsZIP(strFolder))
        {
          CSingleLock lock(m_critSection);
          JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), bind2nd(find_map(), jobID));
          if (i != m_downloadJobs.end() && !CheckHash(URIUtils::GetFileName(strFolder), i->second.hash))
            break;
          AddJob(strFolder);
        }
        else
        {
          // zip extraction job is done
          URIUtils::RemoveSlashAtEnd(strFolder);
          strFolder = URIUtils::AddFileToFolder("special://home/addons/",
                                              URIUtils::GetFileName(strFolder));
          AddonPtr addon;
          bool update=false;
          if (CAddonMgr::Get().LoadAddonDescription(strFolder, addon))
          {
            CStdString strFolder2;
            URIUtils::GetDirectory(strFolder,strFolder2);
            AddonPtr addon2;
            update = CAddonMgr::Get().GetAddon(addon->ID(),addon2);
            CAddonMgr::Get().FindAddons();
            CAddonMgr::Get().GetAddon(addon->ID(),addon);
            ADDONDEPS deps = addon->GetDeps();
            CStdString referer;
            referer.Format("Referer=%s-%s.zip",addon->ID().c_str(),addon->Version().str.c_str());
            for (ADDONDEPS::iterator it  = deps.begin();
                                     it != deps.end();++it)
            {
              if (it->first.Equals("xbmc.metadata"))
                continue;
              if (!CAddonMgr::Get().GetAddon(it->first,addon2))
                InstallAddon(it->first,false,referer);
            }
            if (addon->Type() >= ADDON_VIZ_LIBRARY)
              continue;
            if (update && g_settings.m_bAddonNotifications)
            {
              g_application.m_guiDialogKaiToast.QueueNotification(
                                                  addon->Icon(),
                                                  addon->Name(),
                                                  g_localizeStrings.Get(24065),
                                                  TOAST_DISPLAY_TIME,false,
                                                  TOAST_DISPLAY_TIME);
            }
            else
            {
              if (addon->Type() == ADDON_SKIN)
                m_prompt = addon;
             if (g_settings.m_bAddonNotifications)
                g_application.m_guiDialogKaiToast.QueueNotification(
                                                   addon->Icon(),
                                                   addon->Name(),
                                                   g_localizeStrings.Get(24064),
                                                   TOAST_DISPLAY_TIME,false,
                                                   TOAST_DISPLAY_TIME);
            }
          }
          else
          {
            CStdString addonID = URIUtils::GetFileName(strFolder);
            ReportInstallError(addonID, addonID);
            CLog::Log(LOGERROR,"Could not read addon description of %s", addonID.c_str());
            CFileItemList list;
            list.Add(CFileItemPtr(new CFileItem(strFolder, true)));
            list[0]->Select(true);
            CJobManager::GetInstance().AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete, list, ""), this);
          }
        }
      }
    }
    CAddonMgr::Get().FindAddons();

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendThreadMessage(msg);
  }
  UnRegisterJob(jobID);
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE,g_settings.m_bAddonAutoUpdate);
  SET_CONTROL_SELECTED(GetID(),CONTROL_SHUTUP,g_settings.m_bAddonNotifications);
  CGUIMediaWindow::UpdateButtons();
}

unsigned int CGUIWindowAddonBrowser::AddJob(const CStdString& path)
{
  CFileItemList list;
  CStdString dest="special://home/addons/packages/";
  CStdString package = URIUtils::AddFileToFolder("special://home/addons/packages/",
                                                 URIUtils::GetFileName(path));
  if (URIUtils::HasSlashAtEnd(path))
  {
    dest = "special://home/addons/";
    list.Add(CFileItemPtr(new CFileItem(path,true)));
  }
  else
  {
    // check for cached copy
    if (CFile::Exists(package))
    {
      CStdString archive;
      URIUtils::CreateArchivePath(archive,"zip",package,"");

      CFileItemList archivedFiles;
      CDirectory::GetDirectory(archive, archivedFiles);

      if (archivedFiles.Size() != 1 || !archivedFiles[0]->m_bIsFolder)
      {
        CFile::Delete(package);
        ReportInstallErrorZip(URIUtils::GetFileName(path));
        CLog::Log(LOGERROR, "Package %s is not a valid addon", URIUtils::GetFileName(path).c_str());
        return false;
      }
      list.Add(CFileItemPtr(new CFileItem(archivedFiles[0]->m_strPath,true)));
      // check whether this is an active skin - we need to unload it if so
      CURL url(archivedFiles[0]->m_strPath);
      CStdString addon = url.GetFileName();
      URIUtils::RemoveSlashAtEnd(addon);
      if (g_guiSettings.GetString("lookandfeel.skin") == addon)
      { // we're updating the current skin - we have to unload it first
        CSingleLock lock(m_critSection);
        CAddonMgr::Get().GetAddon(addon, m_prompt);
        m_promptReload = true;
        g_application.getApplicationMessenger().ExecBuiltIn("UnloadSkin", true);
      }
      dest = "special://home/addons/";
    }
    else
    {
      list.Add(CFileItemPtr(new CFileItem(path,false)));
    }
  }

  list[0]->Select(true);
  CFileOperationJob* job = new CFileOperationJob(CFileOperationJob::ActionReplace,
                                                 list,dest);
  return CJobManager::GetInstance().AddJob(job,this);
}

void CGUIWindowAddonBrowser::RegisterJob(const CStdString& id,
                                         unsigned int jobid,
                                         const CStdString& hash)
{
  CSingleLock lock(m_critSection);
  m_downloadJobs.insert(make_pair(id,CDownloadJob(jobid,hash)));
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CGUIWindowAddonBrowser::UnRegisterJob(unsigned int jobID)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), bind2nd(find_map(), jobID));
  if (i != m_downloadJobs.end())
    m_downloadJobs.erase(i);

  AddonPtr prompt;
  bool reload = false;
  if (m_downloadJobs.empty() && m_prompt)
  {
    prompt = m_prompt;
    reload = m_promptReload;
    m_prompt.reset();
    m_promptReload = false;
  }
  lock.Leave();
  PromptForActivation(prompt, reload);
}

void CGUIWindowAddonBrowser::PromptForActivation(const AddonPtr &addon, bool dontPrompt)
{
  if (addon && addon->Type() == ADDON_SKIN)
  {
    if (dontPrompt || CGUIDialogYesNo::ShowAndGetInput(addon->Name(),
                                         g_localizeStrings.Get(24099),"",""))
    {
      g_guiSettings.SetString("lookandfeel.skin",addon->ID().c_str());
      g_application.m_guiDialogKaiToast.ResetTimer();
      g_application.m_guiDialogKaiToast.Close(true);
      g_application.getApplicationMessenger().ExecBuiltIn("ReloadSkin");
    }
  }
  m_prompt.reset();
}

bool CGUIWindowAddonBrowser::GetDirectory(const CStdString& strDirectory,
                                          CFileItemList& items)
{
  bool result;
  if (strDirectory.Equals("addons://downloading/"))
  {
    CAddonDatabase database;
    database.Open();
    CSingleLock lock(m_critSection);
    VECADDONS addons;
    for (JobMap::iterator it = m_downloadJobs.begin(); it != m_downloadJobs.end();++it)
    {
      AddonPtr addon;
      if (database.GetAddon(it->first,addon))
        addons.push_back(addon);
    }
    CURL url(strDirectory);
    CAddonsDirectory::GenerateListing(url,addons,items);
    result = true;
    items.SetProperty("reponame",g_localizeStrings.Get(24067));
    items.m_strPath = strDirectory;

    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItemPtr pItem(new CFileItem(".."));
      pItem->m_strPath = m_history.GetParentPath();
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.AddFront(pItem, 0);
    }

  }
  else
    result = CGUIMediaWindow::GetDirectory(strDirectory,items);

  if (strDirectory.IsEmpty() && !m_downloadJobs.empty())
  {
    CFileItemPtr item(new CFileItem("addons://downloading/",true));
    item->SetLabel(g_localizeStrings.Get(24067));
    item->SetLabelPreformated(true);
    item->SetIconImage("DefaultNetwork.png");
    items.Add(item);
  }

  items.SetContent("addons");

  for (int i=0;i<items.Size();++i)
    SetItemLabel2(items[i]);

  return result;
}

void CGUIWindowAddonBrowser::SetItemLabel2(CFileItemPtr item)
{
  if (!item || item->m_bIsFolder) return;
  CSingleLock lock(m_critSection);
  JobMap::iterator it = m_downloadJobs.find(item->GetProperty("Addon.ID"));
  if (it != m_downloadJobs.end())
  {
    CStdString progress;
    progress.Format(g_localizeStrings.Get(24042).c_str(), it->second.progress);
    item->SetProperty("Addon.Status", progress);
    item->SetProperty("Addon.Downloading", true);
  }
  else
    item->ClearProperty("Addon.Downloading");
  item->SetLabel2(item->GetProperty("Addon.Status"));
  // to avoid the view state overriding label 2
  item->SetLabelPreformated(true);
}

bool CGUIWindowAddonBrowser::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

int CGUIWindowAddonBrowser::SelectAddonID(TYPE type, CStdString &addonID, bool showNone)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (type == ADDON_UNKNOWN || !dialog)
    return 0;

  int selectedIdx = 0;
  ADDON::VECADDONS addons;
  CAddonMgr::Get().GetAddons(type, addons);
  dialog->SetHeading(TranslateType(type, true));
  dialog->Reset();
  dialog->SetUseDetails(true);
  dialog->EnableButton(true, 21452);
  CFileItemList items;
  if (showNone)
  {
    CFileItemPtr item(new CFileItem("", false));
    item->SetLabel(g_localizeStrings.Get(231));
    item->SetLabel2(g_localizeStrings.Get(24040));
    item->SetIconImage("DefaultAddonNone.png");
    item->SetSpecialSort(SORT_ON_TOP);
    items.Add(item);
  }
  for (ADDON::IVECADDONS i = addons.begin(); i != addons.end(); ++i)
    items.Add(CAddonsDirectory::FileItemFromAddon(*i, ""));
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  for (int i = 0; i < items.Size(); ++i)
  {
    if (addonID.Equals(items[i]->GetProperty("Addon.ID")))
      selectedIdx = i;
  }
  dialog->SetItems(&items);
  dialog->SetSelected(selectedIdx);
  dialog->DoModal();
  if (dialog->IsButtonPressed())
  { // switch to the addons browser.
    vector<CStdString> params;
    params.push_back("addons://all/"+TranslateType(type,false)+"/");
    params.push_back("return");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
    return 2;
  }
  if (dialog->GetSelectedLabel() >= 0)
  {
    addonID = dialog->GetSelectedItem().m_strPath;
    return 1;
  }
  return 0;
}

void CGUIWindowAddonBrowser::InstallAddon(const CStdString &addonID, bool force /*= false*/, const CStdString &referer)
{
  // check whether we already have the addon installed
  AddonPtr addon;
  if (!force && CAddonMgr::Get().GetAddon(addonID, addon))
    return;

  // check whether we have it available in a repository
  CAddonDatabase database;
  database.Open();
  if (database.GetAddon(addonID, addon))
  {
    CStdString repo;
    database.GetRepoForAddon(addonID,repo);
    AddonPtr ptr;
    CAddonMgr::Get().GetAddon(repo,ptr);
    RepositoryPtr therepo = boost::dynamic_pointer_cast<CRepository>(ptr);
    CStdString hash;
    if (therepo)
      hash = therepo->GetAddonHash(addon);
    CGUIWindowAddonBrowser* window = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
    if (!window)
      return;
    CStdString path(addon->Path());
    if (!referer.IsEmpty() && URIUtils::IsInternetStream(path))
    {
      CURL url(path);
      url.SetProtocolOptions(referer);
      path = url.Get();
    }
    unsigned int jobID = window->AddJob(path);
    window->RegisterJob(addon->ID(), jobID, hash);
  }
}

void CGUIWindowAddonBrowser::InstallAddonsFromXBMCRepo(const set<CStdString> &addonIDs)
{
  // first check we have the main repository updated...
  AddonPtr addon;
  if (CAddonMgr::Get().GetAddon("repository.xbmc.org", addon))
  {
    VECADDONS addons;
    CAddonDatabase database;
    database.Open();
    if (!database.GetRepository(addon->ID(), addons))
    {
      RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(addon);
      addons = CRepositoryUpdateJob::GrabAddons(repo, false);
    }
  }
  // now install the addons
  for (set<CStdString>::const_iterator i = addonIDs.begin(); i != addonIDs.end(); ++i)
    InstallAddon(*i);
}

CStdString CGUIWindowAddonBrowser::GetStartFolder(const CStdString &dir)
{
  if (dir.Left(9).Equals("addons://"))
    return dir;
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowAddonBrowser::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  CSingleLock lock(m_critSection);
  // find this job
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), bind2nd(find_map(), jobID));
  if (i != m_downloadJobs.end())
  {
    i->second.progress = progress;
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM);
    msg.SetStringParam(i->first);
    g_windowManager.SendThreadMessage(msg);
  }
}

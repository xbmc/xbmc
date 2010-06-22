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
#include "GUIDialogContextMenu.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogFileBrowser.h"
#include "GUIEditControl.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileSystem/AddonsDirectory.h"
#include "utils/FileOperationJob.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "Settings.h"
#include "Application.h"
#include "AddonDatabase.h"
#include "AdvancedSettings.h"

#define CONTROL_AUTOUPDATE 5

using namespace ADDON;
using namespace XFILE;
using namespace std;

CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIMediaWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
  m_thumbLoader.SetNumOfWorkers(1);
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
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID"), addon))
    return;

  if (addon->Type() == ADDON_REPOSITORY)
  {
    buttons.Add(CONTEXT_BUTTON_SCAN,24034);
    buttons.Add(CONTEXT_BUTTON_UPDATE_LIBRARY,24035);
  }

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
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID"), addon))
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

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowAddonBrowser::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item->m_strPath == "install://")
  {
    // pop up filebrowser to grab an installed folder
    VECSOURCES shares = g_settings.m_fileSources;
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
    if (item->GetProperty("Addon.Status").Equals(g_localizeStrings.Get(13413)))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24000),
                                           item->GetProperty("Addon.Name"),
                                           g_localizeStrings.Get(24066),""))
      {
        CSingleLock lock(m_critSection);
        map<CStdString,unsigned int>::iterator it = m_idtojobid.find(item->GetProperty("Addon.ID"));
        if (it != m_idtojobid.end())
        {
          CJobManager::GetInstance().CancelJob(it->second);
          UnRegisterJob(m_idtojob.find(item->GetProperty("Addon.ID"))->second);
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

void CGUIWindowAddonBrowser::OnJobComplete(unsigned int jobID,
                                           bool success, CJob* job2)
{
  if (success)
  {
    CFileOperationJob* job = (CFileOperationJob*)job2;
    if (job->GetAction() == CFileOperationJob::ActionCopy)
    {
      for (int i=0;i<job->GetItems().Size();++i)
      {
        CStdString strFolder = job->GetItems()[i]->m_strPath;
        // zip is downloaded - now extract it
        if (CUtil::IsZIP(strFolder))
        {
          AddJob(strFolder);
        }
        else
        {
          CURL url(strFolder);
          // zip extraction job is done
          if (url.GetProtocol() == "zip")
          {
            CFileItemList list;
            CDirectory::GetDirectory(url.Get(),list);
            CStdString dirname = "";
            for (int i=0;i<list.Size();++i)
            {
              if (list[i]->m_bIsFolder)
              {
                dirname = list[i]->GetLabel();
                break;
              }
            }
            strFolder = CUtil::AddFileToFolder("special://home/addons/",
                                               dirname);
          }
          else
          {
            CUtil::RemoveSlashAtEnd(strFolder);
            strFolder = CUtil::AddFileToFolder("special://home/addons/",
                                               CUtil::GetFileName(strFolder));
          }
          AddonPtr addon;
          bool update=false;
          if (CAddonMgr::Get().LoadAddonDescription(strFolder, addon))
          {
            CStdString strFolder2;
            CUtil::GetDirectory(strFolder,strFolder2);
            AddonPtr addon2;
            update = CAddonMgr::Get().GetAddon(addon->ID(),addon2);
            CAddonMgr::Get().FindAddons();
            CAddonMgr::Get().GetAddon(addon->ID(),addon);
            ADDONDEPS deps = addon->GetDeps();
            for (ADDONDEPS::iterator it  = deps.begin();
                                     it != deps.end();++it)
            {
              if (it->first.Equals("xbmc.metadata"))
                continue;
              if (!CAddonMgr::Get().GetAddon(it->first,addon2))
              {
                CAddonDatabase database;
                database.Open();
                if (database.GetAddon(it->first,addon2))
                  AddJob(addon2->Path());
              }
            }
            if (addon->Type() >= ADDON_VIZ_LIBRARY)
              continue;
            if (update)
            {
              g_application.m_guiDialogKaiToast.QueueNotification(
                                                  CGUIDialogKaiToast::Info,
                                                  addon->Name(),
                                                  g_localizeStrings.Get(24065),
                                                  TOAST_DISPLAY_TIME,false);
            }
            else
            {
              g_application.m_guiDialogKaiToast.QueueNotification(
                                                  CGUIDialogKaiToast::Info,
                                                  addon->Name(),
                                                  g_localizeStrings.Get(24064),
                                                  TOAST_DISPLAY_TIME,false);
            }
          }
        }
      }
    }
    CAddonMgr::Get().FindAddons();

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendThreadMessage(msg);
  }
  UnRegisterJob((CFileOperationJob*)job2);
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE,g_settings.m_bAddonAutoUpdate);
  CGUIMediaWindow::UpdateButtons();
}

pair<CFileOperationJob*,unsigned int> CGUIWindowAddonBrowser::AddJob(const CStdString& path)
{
  CGUIWindowAddonBrowser* that = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
  CFileItemList list;
  CStdString dest="special://home/addons/packages/";
  CStdString package = CUtil::AddFileToFolder("special://home/addons/packages/",
                                              CUtil::GetFileName(path));
  if (CUtil::HasSlashAtEnd(path))
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
      CUtil::CreateArchivePath(archive,"zip",package,"");
      list.Add(CFileItemPtr(new CFileItem(archive,true)));
      dest = "special://home/addons/";
    }
    else
    {
      list.Add(CFileItemPtr(new CFileItem(path,false)));
    }
  }

  list[0]->Select(true);
  CFileOperationJob* job = new CFileOperationJob(CFileOperationJob::ActionCopy,
                                                 list,dest);
  unsigned int id = CJobManager::GetInstance().AddJob(job,that);

  return make_pair(job,id);
}

void CGUIWindowAddonBrowser::RegisterJob(const CStdString& id,
                                         CFileOperationJob* job,
                                         unsigned int jobid)
{
  CSingleLock lock(m_critSection);
  m_idtojob.insert(make_pair(id,job));
  m_idtojobid.insert(make_pair(id,jobid));
  m_jobtoid.insert(make_pair(job,id));
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CGUIWindowAddonBrowser::UnRegisterJob(CFileOperationJob* job)
{
  CSingleLock lock(m_critSection);
  map<CFileOperationJob*,CStdString>::iterator it = m_jobtoid.find((CFileOperationJob*)job);
  if (it != m_jobtoid.end())
  {
    m_idtojob.erase(m_idtojob.find(it->second));
    m_idtojobid.erase(m_idtojobid.find(it->second));
    m_jobtoid.erase(it);
  }
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
    for (map<CStdString,unsigned int>::iterator it = m_idtojobid.begin();
         it != m_idtojobid.end();++it)
    {
      AddonPtr addon;
      if (database.GetAddon(it->first,addon))
        addons.push_back(addon);
    }
    CURL url(strDirectory);
    CAddonsDirectory::GenerateListing(url,addons,items);
    result = true;
    items.SetProperty("Repo.Name",g_localizeStrings.Get(24067));
    items.m_strPath = strDirectory;
  }
  else
    result = CGUIMediaWindow::GetDirectory(strDirectory,items);

  if (strDirectory.IsEmpty() && !m_jobtoid.empty())
  {
    CFileItemPtr item(new CFileItem("addons://downloading/",true));
    item->SetLabel(g_localizeStrings.Get(24067));
    item->SetLabelPreformated(true);
    item->SetThumbnailImage("DefaultNetwork.png");
    items.Add(item);
  }

  CSingleLock lock(m_critSection);
  for (int i=0;i<items.Size();++i)
  {
    if (items[i]->m_bIsFolder)
      continue;
    map<CStdString,CFileOperationJob*>::iterator it = m_idtojob.find(items[i]->GetProperty("Addon.ID"));
    if (it != m_idtojob.end())
      items[i]->SetProperty("Addon.Status",g_localizeStrings.Get(13413));
    items[i]->SetLabel2(items[i]->GetProperty("Addon.Status"));
    // to avoid the view state overriding label 2
    items[i]->SetLabelPreformated(true);
  }

  return result;
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
    items.Add(item);
  }
  for (ADDON::IVECADDONS i = addons.begin(); i != addons.end(); ++i)
    items.Add(CAddonsDirectory::FileItemFromAddon(*i, ""));
  dialog->SetItems(&items);
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

void CGUIWindowAddonBrowser::InstallAddon(const CStdString &addonID, bool force /*= false*/)
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
    CGUIWindowAddonBrowser* window = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
    if (!window)
      return;
    pair<CFileOperationJob*,unsigned int> job = window->AddJob(addon->Path());
    window->RegisterJob(addonID, job.first, job.second);
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

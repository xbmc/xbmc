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
#include "addons/AddonInstaller.h"
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
      CAddonInstaller::Get().InstallFromZip(path);
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
        if (CAddonInstaller::Get().Cancel(item->GetProperty("Addon.ID")))
          Update(m_vecItems->m_strPath);
      }
      return true;
    }

    CGUIDialogAddonInfo::ShowForItem(item);
    return true;
  }

  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE,g_settings.m_bAddonAutoUpdate);
  SET_CONTROL_SELECTED(GetID(),CONTROL_SHUTUP,g_settings.m_bAddonNotifications);
  CGUIMediaWindow::UpdateButtons();
}

bool CGUIWindowAddonBrowser::GetDirectory(const CStdString& strDirectory,
                                          CFileItemList& items)
{
  bool result;
  if (strDirectory.Equals("addons://downloading/"))
  {
    VECADDONS addons;
    CAddonInstaller::Get().GetInstallList(addons);

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

  if (strDirectory.IsEmpty() && CAddonInstaller::Get().IsDownloading())
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
  unsigned int percent;
  if (CAddonInstaller::Get().GetProgress(item->GetProperty("Addon.ID"), percent))
  {
    CStdString progress;
    progress.Format(g_localizeStrings.Get(24042).c_str(), percent);
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

CStdString CGUIWindowAddonBrowser::GetStartFolder(const CStdString &dir)
{
  if (dir.Left(9).Equals("addons://"))
    return dir;
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowAddonBrowser::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
  { // repository update is finished - refresh listing
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendThreadMessage(msg);    
  }
}

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "GUIDialogAddonInfo.h"

#include "addons/AddonManager.h"
#include "AddonDatabase.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogTextViewer.h"
#include "GUIUserMessages.h"
#include "GUIWindowAddonBrowser.h"
#include "GUIWindowManager.h"
#include "URL.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_DISABLE          7
#define CONTROL_BTN_UPDATE           8
#define CONTROL_BTN_SETTINGS         9
#define CONTROL_BTN_CHANGELOG       10

using namespace std;
using namespace ADDON;
using namespace XFILE;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
    : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml")
{
  m_item = CFileItemPtr(new CFileItem);
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
      if (iControl == CONTROL_BTN_UPDATE || iControl == CONTROL_BTN_INSTALL)
      {
        OnInstall();
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_DISABLE)
      {
        OnDisable();
        Close();
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
    }
    break;
default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogAddonInfo::OnInitWindow()
{
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, 
              m_item->GetProperty("Addon.UpdateAvail").Equals("true"));
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, 
              m_item->GetProperty("Addon.Installed").Equals("false"));
  CStdString xbmcPath = _P("special://xbmc/addons");
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_DISABLE, 
              m_item->GetProperty("Addon.Installed").Equals("true") &&
              m_localAddon && !m_localAddon->Path().Mid(0,xbmcPath.size()).Equals(xbmcPath));
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, 
              m_localAddon &&
              m_localAddon->HasSettings());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_CHANGELOG,
              m_addon->Type() != ADDON_REPOSITORY);

  CGUIDialog::OnInitWindow();
  m_changelog = false;
}

void CGUIDialogAddonInfo::OnInstall()
{
  CGUIWindowAddonBrowser* window = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
  pair<CFileOperationJob*,unsigned int> job = window->AddJob(m_addon->Path());
  window->RegisterJob(m_addon->ID(),job.first,job.second);
}

void CGUIDialogAddonInfo::OnDisable()
{
  CGUIWindowAddonBrowser* window = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
  CFileItemList list;
  if (m_localAddon.get())
  {
    list.Add(CFileItemPtr(new CFileItem(m_localAddon->Path(),true)));
    list[0]->Select(true);
    CJobManager::GetInstance().AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete,list,""),window);
  }
  CAddonMgr::Get().RemoveAddon(m_localAddon->ID());
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowAndGetInput(m_localAddon);
}

void CGUIDialogAddonInfo::OnChangeLog()
{
  CGUIDialogTextViewer* pDlgInfo = (CGUIDialogTextViewer*)g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);
  pDlgInfo->SetHeading(g_localizeStrings.Get(24054)+" - "+m_addon->Name());
  if (m_item->GetProperty("Addon.Changelog").IsEmpty())
  {
    pDlgInfo->SetText(g_localizeStrings.Get(13413));
    CFileItemList items;
    items.Add(CFileItemPtr(new CFileItem(m_addon->ChangeLog(),false)));
    items[0]->Select(true);
    m_jobid = CJobManager::GetInstance().AddJob(
      new CFileOperationJob(CFileOperationJob::ActionCopy,items,
                            "special://temp/"),this);
  }
  else
    pDlgInfo->SetText(m_item->GetProperty("Addon.Changelog"));

  m_changelog = true;
  pDlgInfo->DoModal();
  m_changelog = false;
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
  CGUIDialogAddonInfo* dialog = (CGUIDialogAddonInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_INFO);
  if (!dialog)
    return false;
  *dialog->m_item = *item;
  CURL url(item->m_strPath);
  if (url.GetHostName().Equals("enabled"))
  {
    CAddonMgr::Get().GetAddon(item->GetProperty("Addon.ID"),dialog->m_addon);
    dialog->m_item->SetProperty("Addon.Installed","true");
    dialog->m_localAddon = dialog->m_addon;
  }
  else
  {
    if (CAddonMgr::Get().GetAddon(item->GetProperty("Addon.ID"),
                                   dialog->m_addon))
      dialog->m_item->SetProperty("Addon.Installed","true");
    else
      dialog->m_item->SetProperty("Addon.Installed","false");
    dialog->m_localAddon = dialog->m_addon;

    AddonPtr addon;
    CAddonDatabase database;
    database.Open();
    if (database.GetAddon(item->GetProperty("Addon.ID"),addon))
      dialog->m_addon = addon;

    if (!dialog->m_addon)
      return false;
  }
  if (TranslateType(item->GetProperty("Addon.intType")) == ADDON_REPOSITORY)
  {
    CAddonDatabase database;
    database.Open();
    VECADDONS addons;
    database.GetRepository(dialog->m_addon->ID(),addons);
    int tot=0;
    for (int i = ADDON_UNKNOWN+1;i<ADDON_VIZ_LIBRARY;++i)
    {
      int num=0;
      for (unsigned int j=0;j<addons.size();++j)
      {
        if (addons[j]->Type() == (TYPE)i)
          ++num;
      }
      dialog->m_item->SetProperty(CStdString("Repo.")+TranslateType((TYPE)i),num);
      tot += num;
    }
    dialog->m_item->SetProperty("Repo.Addons",tot);
  }
  dialog->DoModal(); 
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
    if (file.Open("special://temp/"+
      CUtil::GetFileName(((CFileOperationJob*)job)->GetItems()[0]->m_strPath)))
    {
      char* temp = new char[(size_t)file.GetLength()+1];
      file.Read(temp,file.GetLength());
      temp[file.GetLength()] = '\0';
      m_item->SetProperty("Addon.Changelog",temp);
      pDlgInfo->SetText(temp);
      delete[] temp;
    }
  }
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, WINDOW_DIALOG_TEXT_VIEWER, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}


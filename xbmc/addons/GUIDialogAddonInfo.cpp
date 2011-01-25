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
#include "dialogs/GUIDialogYesNo.h"
#include "addons/AddonManager.h"
#include "AddonDatabase.h"
#include "FileItem.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "utils/URIUtils.h"
#include "addons/AddonInstaller.h"
#include "Application.h"

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_ENABLE           7
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
        else if (CGUIDialogYesNo::ShowAndGetInput(24037, 750, 0, 0))
        {
          OnUninstall();
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_ENABLE)
      {
        OnEnable(!m_item->GetProperty("Addon.Enabled").Equals("true"));
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
  UpdateControls();
  CGUIDialog::OnInitWindow();
  m_changelog = false;
}

void CGUIDialogAddonInfo::UpdateControls()
{
  CStdString xbmcPath = _P("special://xbmc/addons");
  bool isInstalled = NULL != m_localAddon.get();
  bool isSystem = isInstalled && m_localAddon->Path().Left(xbmcPath.size()).Equals(xbmcPath);
  bool isEnabled = isInstalled && m_item->GetProperty("Addon.Enabled").Equals("true");
  bool isUpdatable = isInstalled && m_item->GetProperty("Addon.UpdateAvail").Equals("true");
  // TODO: System addons should be able to be disabled
  bool canDisable = isInstalled && (!isSystem || m_localAddon->Type() == ADDON_PVRDLL) && !m_localAddon->IsInUse();
  bool canInstall = !isInstalled && m_item->GetProperty("Addon.Broken").IsEmpty();
                     
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canDisable || canInstall);
  SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, isInstalled ? 24037 : 24038);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, canDisable);
  SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, isEnabled ? 24021 : 24022);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, isUpdatable);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && m_localAddon->HasSettings());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_CHANGELOG, m_addon->Type() != ADDON_REPOSITORY);
}

void CGUIDialogAddonInfo::OnUpdate()
{
  CStdString referer;
  referer.Format("Referer=%s-%s.zip",m_localAddon->ID().c_str(),m_localAddon->Version().str.c_str());
  CAddonInstaller::Get().Install(m_addon->ID(), true, referer); // force install
  Close();
}

void CGUIDialogAddonInfo::OnInstall()
{
  CAddonInstaller::Get().Install(m_addon->ID());
  Close();
}

void CGUIDialogAddonInfo::OnUninstall()
{
  if (!m_localAddon.get())
    return;

  // ensure the addon isn't disabled in our database
  CAddonDatabase database;
  database.Open();
  database.DisableAddon(m_localAddon->ID(), false);

  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(m_localAddon->Path(),true)));
  list[0]->Select(true);
  CJobManager::GetInstance().AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete,list,""),NULL);
  CAddonMgr::Get().RemoveAddon(m_localAddon->ID());
  Close();
}

void CGUIDialogAddonInfo::OnEnable(bool enable)
{
  if (!m_localAddon.get())
    return;

  CStdString xbmcPath = _P("special://xbmc/addons");
  CAddonDatabase database;
  database.Open();
  if (m_localAddon->Type() == ADDON_PVRDLL && m_localAddon->Path().Left(xbmcPath.size()).Equals(xbmcPath))
    database.EnableSystemPVRAddon(m_localAddon->ID(), enable);
  else
    database.DisableAddon(m_localAddon->ID(), !enable);

  if (m_localAddon->Type() == ADDON_PVRDLL && enable)
    g_application.StartPVRManager();

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
  if (!dialog->SetItem(item))
    return false;

  dialog->DoModal(); 
  return true;
}

bool CGUIDialogAddonInfo::SetItem(const CFileItemPtr& item)
{
  *m_item = *item;

  // grab the local addon, if it's available
  m_addon.reset();
  if (CAddonMgr::Get().GetAddon(item->GetProperty("Addon.ID"), m_addon)) // sets m_addon if installed regardless of enabled state
    m_item->SetProperty("Addon.Enabled", "true");
  else
    m_item->SetProperty("Addon.Enabled", "false");
  m_item->SetProperty("Addon.Installed", m_addon ? "true" : "false");
  m_localAddon = m_addon;

  if (!m_addon)
  { // coming from a repository
    CAddonDatabase database;
    database.Open();
    if (!database.GetAddon(item->GetProperty("Addon.ID"),m_addon))
      return false; // can't find the addon
  }
  if (TranslateType(item->GetProperty("Addon.intType")) == ADDON_REPOSITORY)
  {
    CAddonDatabase database;
    database.Open();
    VECADDONS addons;
    database.GetRepository(m_addon->ID(), addons);
    int tot=0;
    for (int i = ADDON_UNKNOWN+1;i<ADDON_VIZ_LIBRARY;++i)
    {
      int num=0;
      for (unsigned int j=0;j<addons.size();++j)
      {
        if (addons[j]->Type() == (TYPE)i)
          ++num;
      }
      m_item->SetProperty(CStdString("Repo.") + TranslateType((TYPE)i), num);
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
    if (file.Open("special://temp/"+
      URIUtils::GetFileName(((CFileOperationJob*)job)->GetItems()[0]->m_strPath)))
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


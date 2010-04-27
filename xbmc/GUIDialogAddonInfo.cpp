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
#include "GUIDialogAddonSettings.h"
#include "GUIWindowAddonBrowser.h"
#include "GUIWindowManager.h"
#include "URL.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"

#define CONTROL_BTN_INSTALL          6
#define CONTROL_BTN_DISABLE          7
#define CONTROL_BTN_UPDATE           8
#define CONTROL_BTN_SETTINGS         9

using namespace std;
using namespace ADDON;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
    : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml")
{
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
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, 
                  m_item->GetProperty("Addon.UpdateAvail").Equals("true"));
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, 
                  m_item->GetProperty("Addon.Installed").Equals("false"));
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_DISABLE, 
                  m_item->GetProperty("Addon.Installed").Equals("true") &&
        !m_item->GetProperty("Addon.Path").Mid(0,15).Equals("special://xbmc/"));
      CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, 
                  m_item->GetProperty("Addon.Installed").Equals("true") &&
                  m_addon->HasSettings());

      return true;
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
    }
    break;
default:
    break;
  }

  return CGUIDialog::OnMessage(message);
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
  list.Add(CFileItemPtr(new CFileItem(m_addon->Path(),true)));
  list[0]->Select(true);
  CJobManager::GetInstance().AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete,list,""),window);
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowAndGetInput(m_addon);
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
  CGUIDialogAddonInfo* dialog = (CGUIDialogAddonInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_INFO);
  if (!dialog)
    return false;
  dialog->m_item = CFileItemPtr(new CFileItem(*item));
  CURL url(item->m_strPath);
  if (url.GetHostName().Equals("enabled"))
  {
    CAddonMgr::Get()->GetAddon(item->GetProperty("Addon.ID"),dialog->m_addon);
    dialog->m_item->SetProperty("Addon.Installed","true");
  }
  else
  {
    if (CAddonMgr::Get()->GetAddon(item->GetProperty("Addon.ID"),
                                   dialog->m_addon))
      dialog->m_item->SetProperty("Addon.Installed","true");
    else
      dialog->m_item->SetProperty("Addon.Installed","false");
    if (!dialog->m_addon)
    {
      CAddonDatabase database;
      database.Open();
      database.GetAddon(item->GetProperty("Addon.ID"),dialog->m_addon);
    }
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


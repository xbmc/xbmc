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
#include "GUIDialogContextMenu.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogYesNo.h"
#include "GUIEditControl.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "utils/FileOperationJob.h"
#include "utils/JobManager.h"
#include "Settings.h"
#include "Application.h"
#include "AddonDatabase.h"

#define CONTROL_AUTOUPDATE 5

using namespace ADDON;
using namespace XFILE;

CGUIWindowAddonBrowser::CGUIWindowAddonBrowser(void)
: CGUIMediaWindow(WINDOW_ADDON_BROWSER, "AddonBrowser.xml")
{
}

CGUIWindowAddonBrowser::~CGUIWindowAddonBrowser()
{
}

bool CGUIWindowAddonBrowser::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_vecItems->m_strPath = "";
      SetHistoryForPath(m_vecItems->m_strPath);
    }
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_AUTOUPDATE)
      {
        g_settings.m_bAddonAutoUpdate = !g_settings.m_bAddonAutoUpdate;
        g_settings.Save();
        return true;
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
  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), 
                                  addon, type, false)) 
    return;

  if (addon->HasSettings())
    buttons.Add(CONTEXT_BUTTON_SETTINGS,24020);
}

bool CGUIWindowAddonBrowser::OnContextButton(int itemNumber,
                                             CONTEXT_BUTTON button)
{
  CFileItemPtr pItem = m_vecItems->Get(itemNumber);
  TYPE type = TranslateType(pItem->GetProperty("Addon.Type"));
  AddonPtr addon;
  if (!CAddonMgr::Get()->GetAddon(pItem->GetProperty("Addon.ID"), 
                                  addon, type, false)) 
    return false;
  if (button == CONTEXT_BUTTON_SETTINGS)
    return CGUIDialogAddonSettings::ShowAndGetInput(addon);

  return false;
}

bool CGUIWindowAddonBrowser::OnClick(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item->m_bIsFolder)
  {
    AddonPtr addon;
    if (CAddonMgr::Get()->GetAddon(item->GetProperty("Addon.ID"),addon))
    {
      if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24000),
                                           addon->Name(),
                                           g_localizeStrings.Get(24060),""))
      {
        CFileItemList list;
        list.Add(CFileItemPtr(new CFileItem(CUtil::AddFileToFolder("special://home/addons",item->GetProperty("Addon.ID")),true)));
        list[0]->Select(true);
        CJobManager::GetInstance().AddJob(new CFileOperationJob(CFileOperationJob::ActionDelete,list,"special://home/addons/"),this);
      }
    }
    else
    {
      if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(24000),
                                           item->GetProperty("Addon.Name"),
                                           g_localizeStrings.Get(24059),""))
      {
        AddJob(item->GetProperty("Addon.Path"));
      }
    } 
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
          else // not reachable - in case we decide to allow non-zipped repos
          {
            CUtil::RemoveSlashAtEnd(strFolder);
            strFolder = CUtil::AddFileToFolder("special://home/addons/",
                                               CUtil::GetFileName(strFolder));
          }
          AddonPtr addon;
          if (CAddonMgr::AddonFromInfoXML(strFolder,addon))
          {
            CStdString strFolder2;
            CUtil::GetDirectory(strFolder,strFolder2);
            for (ADDONDEPS::iterator it  = addon->GetDeps().begin();
                                     it != addon->GetDeps().end();++it)
            {
              AddonPtr addon2;
              if (!CAddonMgr::Get()->GetAddon(it->first,addon2))
              {
                CAddonDatabase database;
                database.Open();
                database.GetAddon(it->first,addon2);
                AddJob(addon2->Path());
              }
            }
            if (addon->Type() >= ADDON_VIZ_LIBRARY)
              continue;
            AddonPtr addon2;
            if (CAddonMgr::Get()->GetAddon(addon->ID(),addon2))
            {
              g_application.m_guiDialogKaiToast.QueueNotification(
                                                  CGUIDialogKaiToast::Info,
                                                  addon->Name(),
                                                  g_localizeStrings.Get(24065));
            }
            else
            {
              g_application.m_guiDialogKaiToast.QueueNotification(
                                                  CGUIDialogKaiToast::Info,
                                                  addon->Name(),
                                                  g_localizeStrings.Get(24064));
            }
          }
        }
      }
    }
    CAddonMgr::Get()->FindAddons();

    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
    g_windowManager.SendThreadMessage(msg);
  }
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE,g_settings.m_bAddonAutoUpdate);
  CGUIMediaWindow::UpdateButtons();  
}

void CGUIWindowAddonBrowser::AddJob(const CStdString& path)
{
  CGUIWindowAddonBrowser* that = (CGUIWindowAddonBrowser*)g_windowManager.GetWindow(WINDOW_ADDON_BROWSER);
  CFileItemList list;
  CStdString dest="special://home/addons/packages/";
  CStdString package = CUtil::AddFileToFolder("special://home/addons/packages/",
                                              CUtil::GetFileName(path));
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

  CUtil::GetDirectory(path,package);
  list[0]->SetProperty("Repo.Path",package);
  list[0]->Select(true);
  CJobManager::GetInstance().AddJob(
         new CFileOperationJob(CFileOperationJob::ActionCopy,list,dest),that);
}


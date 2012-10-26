/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowAddonBrowser.h"
#include "addons/AddonManager.h"
#include "addons/Repository.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"
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
#include "LangInfo.h"

#define CONTROL_AUTOUPDATE    5
#define CONTROL_SHUTUP        6
#define CONTROL_FOREIGNFILTER 7

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
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().IsEmpty())
        m_vecItems->SetPath("");
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
      else if (iControl == CONTROL_FOREIGNFILTER)
      {
        g_settings.m_bAddonForeignFilter = !g_settings.m_bAddonForeignFilter;
        g_settings.Save();
        Refresh();
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
          if (!m_vecItems->Get(iItem)->GetProperty("Addon.ID").empty())
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
  if (pItem->GetPath().Equals("addons://enabled/"))
    buttons.Add(CONTEXT_BUTTON_SCAN,24034);
  
  AddonPtr addon;
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID").asString(), addon, ADDON_UNKNOWN, false)) // allow disabled addons
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
  if (pItem->GetPath().Equals("addons://enabled/"))
  {
    if (button == CONTEXT_BUTTON_SCAN)
    {
      CAddonMgr::Get().FindAddons();
      return true;
    }
  }
  AddonPtr addon;
  if (!CAddonMgr::Get().GetAddon(pItem->GetProperty("Addon.ID").asString(), addon, ADDON_UNKNOWN, false)) // allow disabled addons
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
    CAddonInstaller::Get().UpdateRepos(true);
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
  if (item->GetPath() == "addons://install/")
  {
    // pop up filebrowser to grab an installed folder
    VECSOURCES shares = g_settings.m_fileSources;
    g_mediaManager.GetLocalDrives(shares);
    g_mediaManager.GetNetworkLocations(shares);
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
                                           item->GetProperty("Addon.Name").asString(),
                                           g_localizeStrings.Get(24066),""))
      {
        if (CAddonInstaller::Get().Cancel(item->GetProperty("Addon.ID").asString()))
          Refresh();
      }
      return true;
    }

    CGUIDialogAddonInfo::ShowForItem(item);
    return true;
  }
  if (item->GetPath().Equals("addons://search/"))
    return Update(item->GetPath());

  return CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowAddonBrowser::UpdateButtons()
{
  SET_CONTROL_SELECTED(GetID(),CONTROL_AUTOUPDATE,g_settings.m_bAddonAutoUpdate);
  SET_CONTROL_SELECTED(GetID(),CONTROL_SHUTUP,g_settings.m_bAddonNotifications);
  SET_CONTROL_SELECTED(GetID(),CONTROL_FOREIGNFILTER,g_settings.m_bAddonForeignFilter);
  CGUIMediaWindow::UpdateButtons();
}

static bool FilterVar(bool valid, const CVariant& variant,
                                  const std::string& check)
{
  if (!valid)
    return false;

  if (variant.isNull() || variant.asString().empty())
    return false;

  std::string regions = variant.asString();
  return regions.find(check) == std::string::npos;
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
    items.SetPath(strDirectory);

    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItemPtr pItem(new CFileItem(".."));
      pItem->SetPath(m_history.GetParentPath());
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.AddFront(pItem, 0);
    }

  }
  else
  {
    result = CGUIMediaWindow::GetDirectory(strDirectory,items);
    if (g_settings.m_bAddonForeignFilter)
    {
      int i=0;
      while (i < items.Size())
      {
        if (!FilterVar(g_settings.m_bAddonForeignFilter,
                      items[i]->GetProperty("Addon.Language"), "en") ||
            !FilterVar(g_settings.m_bAddonForeignFilter,
                      items[i]->GetProperty("Addon.Language"),
                      g_langInfo.GetLanguageLocale()))
        {
          i++;
        }
        else
          items.Remove(i);
      }
    }
  }

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
  if (CAddonInstaller::Get().GetProgress(item->GetProperty("Addon.ID").asString(), percent))
  {
    CStdString progress;
    progress.Format(g_localizeStrings.Get(24042).c_str(), percent);
    item->SetProperty("Addon.Status", progress);
    item->SetProperty("Addon.Downloading", true);
  }
  else
    item->ClearProperty("Addon.Downloading");
  item->SetLabel2(item->GetProperty("Addon.Status").asString());
  // to avoid the view state overriding label 2
  item->SetLabelPreformated(true);
}

bool CGUIWindowAddonBrowser::Update(const CStdString &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

int CGUIWindowAddonBrowser::SelectAddonID(TYPE type, CStdString &addonID, bool showNone /*= false*/)
{
  vector<ADDON::TYPE> types;
  types.push_back(type);
  return SelectAddonID(types, addonID, showNone);
}

int CGUIWindowAddonBrowser::SelectAddonID(ADDON::TYPE type, CStdStringArray &addonIDs, bool showNone /*= false*/, bool multipleSelection /*= true*/)
{
  vector<ADDON::TYPE> types;
  types.push_back(type);
  return SelectAddonID(types, addonIDs, showNone, multipleSelection);
}

int CGUIWindowAddonBrowser::SelectAddonID(const vector<ADDON::TYPE> &types, CStdString &addonID, bool showNone /*= false*/)
{
  CStdStringArray addonIDs;
  if (!addonID.IsEmpty())
    addonIDs.push_back(addonID);
  int retval = SelectAddonID(types, addonIDs, showNone, false);
  if (addonIDs.size() > 0)
    addonID = addonIDs.at(0);
  else
    addonID = "";
  return retval;
}

int CGUIWindowAddonBrowser::SelectAddonID(const vector<ADDON::TYPE> &types, CStdStringArray &addonIDs, bool showNone /*= false*/, bool multipleSelection /*= true*/)
{
  CGUIDialogSelect *dialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog)
    return 0;

  CFileItemList items;
  CStdString heading;
  int iTypes = 0;
  for (vector<ADDON::TYPE>::const_iterator it = types.begin(); it != types.end(); ++it)
  {
    if (*it == ADDON_UNKNOWN)
      continue;
    ADDON::VECADDONS addons;
    iTypes++;
    if (*it == ADDON_AUDIO)
      CAddonsDirectory::GetScriptsAndPlugins("audio",addons);
    else if (*it == ADDON_EXECUTABLE)
      CAddonsDirectory::GetScriptsAndPlugins("executable",addons);
    else if (*it == ADDON_IMAGE)
      CAddonsDirectory::GetScriptsAndPlugins("image",addons);
    else if (*it == ADDON_VIDEO)
      CAddonsDirectory::GetScriptsAndPlugins("video",addons);
    else
      CAddonMgr::Get().GetAddons(*it, addons);
    for (ADDON::IVECADDONS it2 = addons.begin() ; it2 != addons.end() ; ++it2)
    {
      CFileItemPtr item(CAddonsDirectory::FileItemFromAddon(*it2, ""));
      if (!items.Contains(item->GetPath()))
        items.Add(item);
    }

    if (!heading.IsEmpty())
      heading += ", ";
    heading += TranslateType(*it, true);
  }

  if (iTypes == 0)
    return 0;

  dialog->SetHeading(heading);
  dialog->Reset();
  dialog->SetUseDetails(true);
  if (multipleSelection)
    showNone = false;
  if (multipleSelection || iTypes > 1)
    dialog->EnableButton(true, 186);
  else
    dialog->EnableButton(true, 21452);
  if (showNone)
  {
    CFileItemPtr item(new CFileItem("", false));
    item->SetLabel(g_localizeStrings.Get(231));
    item->SetLabel2(g_localizeStrings.Get(24040));
    item->SetIconImage("DefaultAddonNone.png");
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  items.Sort(SORT_METHOD_LABEL, SortOrderAscending);

  if (addonIDs.size() > 0)
  {
    for (CStdStringArray::const_iterator it = addonIDs.begin(); it != addonIDs.end() ; it++)
    {
      CFileItemPtr item = items.Get(*it);
      if (item)
        item->Select(true);
    }
  }
  dialog->SetItems(&items);
  dialog->SetMultiSelection(multipleSelection);
  dialog->DoModal();
  if (!multipleSelection && iTypes == 1 && dialog->IsButtonPressed())
  { // switch to the addons browser.
    vector<CStdString> params;
    params.push_back("addons://all/"+TranslateType(types[0],false)+"/");
    params.push_back("return");
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
    return 2;
  }
  if (!multipleSelection && dialog->GetSelectedLabel() == -1)
    return 0;
  addonIDs.clear();
  const CFileItemList& list = dialog->GetSelectedItems();
  for (int i = 0 ; i < list.Size() ; i++)
    addonIDs.push_back(list.Get(i)->GetPath());
  return 1;
}

CStdString CGUIWindowAddonBrowser::GetStartFolder(const CStdString &dir)
{
  if (dir.Left(9).Equals("addons://"))
    return dir;
  return CGUIMediaWindow::GetStartFolder(dir);
}

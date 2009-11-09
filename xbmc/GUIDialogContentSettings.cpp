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

#include "GUIDialogContentSettings.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogOK.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "utils/IAddon.h"
#include "FileItem.h"
#include "VideoDatabase.h"
#include "VideoInfoScanner.h"
#include "GUISettings.h"

#define CONTROL_CONTENT_TYPE        3
#define CONTROL_SCRAPER_LIST        4
#define CONTROL_SCRAPER_SETTINGS    6
#define CONTROL_START              30

using namespace DIRECTORY;
using namespace std;
using namespace ADDON;

CGUIDialogContentSettings::CGUIDialogContentSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_CONTENT_SETTINGS, "DialogContentSettings.xml")
{
  m_bNeedSave = false;
  m_content = CONTENT_NONE;
  m_vecItems = new CFileItemList;
}

CGUIDialogContentSettings::~CGUIDialogContentSettings(void)
{
  delete m_vecItems;
}

bool CGUIDialogContentSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_scrapers.clear();
      m_vecItems->Clear();
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  /*case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == CONTROL_SCRAPER_LIST)
      {
        m_scraper = boost::dynamic_pointer_cast<CScraper>(m_scrapers[m_content][message.GetParam1()]);
      }
    }
    break;*/

  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_CONTENT_TYPE)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_CONTENT_TYPE);
      g_windowManager.SendMessage(msg);
      m_content = (CONTENT_TYPE) msg.GetParam1();
      SetupPage();
    }
    if (iControl == CONTROL_SCRAPER_LIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SCRAPER_LIST);
      g_windowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();

      AddonPtr last = m_scraper;
      m_scraper = m_scrapers[m_content][iSelected];
      m_bNeedSave = m_scraper != last;
      SET_CONTROL_FOCUS(30,0);
    }
    if (iControl == CONTROL_SCRAPER_SETTINGS)
    {
      if (CGUIDialogAddonSettings::ShowAndGetInput(m_scraper))
      { // settings are changed, but not saved
        m_bNeedSave = true;
        return true;
      }
      return false;
    }
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogContentSettings::OnWindowLoaded()
{//TODO right order?
  CGUIDialogSettings::OnWindowLoaded();
  FillContentTypes();
  SetupPage();
}

void CGUIDialogContentSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_CONTENT_TYPE);
  g_windowManager.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_CONTENT_TYPE);

  msg2.SetLabel("<"+g_localizeStrings.Get(231)+">");
  msg2.SetParam1(0);
  g_windowManager.SendMessage(msg2);

  CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 1);

  if (m_content == CONTENT_NONE)
  {
    SET_CONTROL_HIDDEN(CONTROL_SCRAPER_LIST);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_SCRAPER_LIST);
  }
}

void CGUIDialogContentSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  if (m_bExclude)
  {
    AddBool(1,20380,&m_bExclude);
    return;
  }

  switch (m_content)
  {
  case CONTENT_TVSHOWS:
    {
      AddBool(1,20345,&m_bRunScan);
      AddBool(2,20379,&m_bSingleItem);
      AddBool(3,20432,&m_bNoUpdate);
      break;
    }
  case CONTENT_MOVIES:
    {
      AddBool(1,20345,&m_bRunScan);
      AddBool(2,20330,&m_bUseDirNames);
      AddBool(3,20346,&m_bScanRecursive);
      AddBool(4,20383,&m_bSingleItem, m_bUseDirNames);
      AddBool(5,20432,&m_bNoUpdate);
      break;
    }
  case CONTENT_MUSICVIDEOS:
    {
      AddBool(1,20345,&m_bRunScan);
      AddBool(2,20346,&m_bScanRecursive);
      AddBool(3,20432,&m_bNoUpdate);
    }
  case CONTENT_ALBUMS:
    {
      AddBool(1,20345,&m_bRunScan);
    }
  default:
    break;
  }
}

void CGUIDialogContentSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == 3) // scan recursive
  {
    m_bSingleItem = false;
    UpdateSetting(4);
  }
  else if (setting.id == 4)
  {
    m_bScanRecursive = false;
    UpdateSetting(3);
  }

  m_bNeedSave = true;
}

void CGUIDialogContentSettings::OnOkay()
{
}

void CGUIDialogContentSettings::OnCancel()
{
  m_bNeedSave = false;
}

void CGUIDialogContentSettings::OnInitWindow()
{
  m_bNeedSave = false;

  CGUIDialogSettings::OnInitWindow();
  SET_CONTROL_FOCUS(CONTROL_CONTENT_TYPE,0);
}

void CGUIDialogContentSettings::FillContentTypes()
{
  CAddonMgr::Get()->LoadAddonsXML(ADDON_SCRAPER);
  FillContentTypes(CONTENT_MOVIES);
  FillContentTypes(CONTENT_TVSHOWS);
  FillContentTypes(CONTENT_MUSICVIDEOS);
  FillContentTypes(CONTENT_ALBUMS);
  FillContentTypes(CONTENT_ARTISTS);
}

void CGUIDialogContentSettings::FillContentTypes(const CONTENT_TYPE &content)
{
  // grab all scrapers which support this content-type
  VECADDONS addons;
  CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, addons, content);

  if (addons.empty())
  {
    return;
  }

  AddonPtr addon;
  CStdString defaultUUID;
  CAddonMgr::Get()->GetDefaultScraper(addon, content);
  if (addon)
    defaultUUID = addon->UUID();

  for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
  {
    bool isDefault = ((*it)->UUID() == defaultUUID);
    map<CONTENT_TYPE,VECADDONS>::iterator iter=m_scrapers.find(content);

    AddonPtr scraper = (*it)->Clone();

    if (m_scraper && m_scraper->Parent() == (*it)->UUID())
    { // don't overwrite preconfigured scraper
      scraper = m_scraper;
    }

    if (iter != m_scrapers.end())
    {      
      if (isDefault)
        iter->second.insert(iter->second.begin(), scraper);
      else
        iter->second.push_back(scraper);
    }
    else
    {
      VECADDONS vec;
      vec.push_back(scraper);
      m_scrapers.insert(make_pair(content,vec));
    }
  }
}
 
void CGUIDialogContentSettings::FillListControl()
{
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
  OnMessage(msgReset);
  int iIndex=0;
  m_vecItems->Clear();

  if (m_scrapers.size() == 0)
    return;

  for (IVECADDONS iter=m_scrapers.find(m_content)->second.begin();iter!=m_scrapers.find(m_content)->second.end();++iter)
  {
    CFileItemPtr item(new CFileItem((*iter)->Name()));
    item->m_strPath = (*iter)->UUID();
    item->SetThumbnailImage((*iter)->Icon());
    if (m_scraper && (*iter)->UUID() == m_scraper->UUID())
    {
      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SCRAPER_LIST, iIndex);
      OnMessage(msg2);
      item->Select(true);
    }
    m_vecItems->Add(item);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SCRAPER_LIST, 0, 0, item);
    OnMessage(msg);
    iIndex++;
  }
}

CFileItemPtr CGUIDialogContentSettings::GetCurrentListItem(int offset)
{
  int currentItem = -1;
  if(m_bExclude)
    return CFileItemPtr();
  for (int i=0;i<m_vecItems->Size();++i )
  {
    if (m_vecItems->Get(i)->IsSelected())
    {
      currentItem = i;
      break;
    }
  }

  if (currentItem == -1)
    return CFileItemPtr();

  int item = (currentItem + offset) % m_vecItems->Size();
  if (item < 0) item += m_vecItems->Size();
  return m_vecItems->Get(item);
}

bool CGUIDialogContentSettings::ShowForDirectory(const CStdString& strDirectory, ADDON::CScraperPtr& scraper, VIDEO::SScanSettings& settings, bool& bRunScan)
{
  CVideoDatabase database;
  database.Open();
  int iFound;
  database.GetScraperForPath(strDirectory,scraper,settings, iFound);
  bool bResult = Show(scraper,settings,bRunScan);
  if (bResult)
    database.SetScraperForPath(strDirectory,scraper,settings);

  return bResult;
}

bool CGUIDialogContentSettings::Show(ADDON::CScraperPtr& scraper, bool& bRunScan, CONTENT_TYPE musicContext/*=CONTENT_NONE*/)
{
  VIDEO::SScanSettings dummy;
  dummy.recurse = -1;
  dummy.parent_name = false;
  dummy.parent_name_root = false;
  dummy.noupdate = false;
  return Show(scraper,dummy,bRunScan,musicContext);
}

bool CGUIDialogContentSettings::Show(ADDON::CScraperPtr& scraper, VIDEO::SScanSettings& settings, bool& bRunScan, CONTENT_TYPE musicContext/*=CONTENT_NONE*/)
{
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (!dialog)
    return false;

  if (musicContext != CONTENT_NONE)
  {
    dialog->m_content = musicContext;
  }

  dialog->m_scraper = scraper;
  dialog->m_bRunScan = bRunScan;
  dialog->m_bScanRecursive = (settings.recurse > 0 && !settings.parent_name) || (settings.recurse > 1 && settings.parent_name);
  dialog->m_bUseDirNames   = settings.parent_name;
  dialog->m_bExclude       = settings.exclude; 
  dialog->m_bSingleItem    = settings.parent_name_root;
  dialog->m_bNoUpdate      = settings.noupdate;
  dialog->m_bNeedSave = false;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    scraper = boost::dynamic_pointer_cast<CScraper>(dialog->m_scraper);
    CONTENT_TYPE content = dialog->m_content;
    if (!scraper || content == CONTENT_NONE)
    {
      bRunScan = false;
      settings.exclude = dialog->m_bExclude;
    }
    else 
    {
      settings.exclude = false;
      bRunScan = dialog->m_bRunScan;
      scraper->m_pathContent = content;

      if (content == CONTENT_TVSHOWS)
      {
        settings.parent_name = dialog->m_bSingleItem;
        settings.parent_name_root = dialog->m_bSingleItem;
        settings.recurse = 0;
      }
      else if (content == CONTENT_MOVIES)
      {
        if (dialog->m_bUseDirNames)
        {
          settings.parent_name = true;
          settings.parent_name_root = false;
          settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 1;

          if (dialog->m_bSingleItem)
          {
            settings.parent_name_root = true;
            settings.recurse = 0;
          }
        }
        else
        {
          settings.parent_name = false;
          settings.parent_name_root = false;
          settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;
        }
      }
      else if (content == CONTENT_MUSICVIDEOS)
      {
        settings.parent_name = false;
        settings.parent_name_root = false;
        settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;
      }
    }

    return true;
  }
  return false;
}


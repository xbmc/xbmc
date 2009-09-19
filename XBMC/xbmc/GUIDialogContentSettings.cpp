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

#include "stdafx.h"
#include "GUIDialogContentSettings.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogOK.h"
#include "GUISettings.h"
#include "Util.h"
#include "VideoDatabase.h"
#include "VideoInfoScanner.h"
#include "FileSystem/Directory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "GUIWindowManager.h"
#include "utils/ScraperParser.h"
#include "utils/IAddon.h"
#include "FileItem.h"

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

  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == CONTROL_SCRAPER_LIST)
      {
        m_scraper = boost::dynamic_pointer_cast<CScraper>(m_scrapers[m_content][message.GetParam1()]);
      }
    }
    break;

  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    if (iControl == 500)
      Close();
    if (iControl == 501)
    {
      m_bNeedSave = false;
      Close();
    }

    if (iControl == CONTROL_CONTENT_TYPE)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_CONTENT_TYPE);
      m_gWindowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();

      m_bNeedSave = true;
      CStdString strLabel;
      switch (iSelected)
      {
      case 0: m_scraper.reset();
              OnSettingChanged(0);
              SetupPage();
              break;
      case 1: strLabel = g_localizeStrings.Get(20342);
              m_content = CONTENT_MOVIES;
              if (CAddonMgr::Get()->GetDefaultScraper(m_scraper, CONTENT_MOVIES))
              {
                CreateSettings();
                SetupPage();
              }
              break;
      case 2: strLabel = g_localizeStrings.Get(20343);
              m_content = CONTENT_TVSHOWS;
              if (CAddonMgr::Get()->GetDefaultScraper(m_scraper, CONTENT_TVSHOWS))
              {
                CreateSettings();
                SetupPage();
              }
              break;
      case 3: strLabel = g_localizeStrings.Get(20389);
              m_content = CONTENT_MUSICVIDEOS;
              if (CAddonMgr::Get()->GetDefaultScraper(m_scraper, CONTENT_MUSICVIDEOS))
              {
                CreateSettings();
                SetupPage();
              }              
              break;
      case 4: strLabel = g_localizeStrings.Get(132);
              m_content = CONTENT_ALBUMS;
              if (CAddonMgr::Get()->GetDefaultScraper(m_scraper, CONTENT_ALBUMS))
              {
                CreateSettings();
                SetupPage();
              }
              break;
      }
    }
    if (iControl == CONTROL_SCRAPER_LIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SCRAPER_LIST);
      m_gWindowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();

      m_scraper = m_scrapers[m_content][iSelected];
      FillListControl();
      SET_CONTROL_FOCUS(30,0);
      m_bNeedSave = true;
    }
    if (iControl == CONTROL_SCRAPER_SETTINGS)
    {
      if (CGUIDialogAddonSettings::ShowAndGetInput(m_scraper))
      {
        m_bNeedSave = true;
        return true;
      }
      return false;
    }
    if (m_scraper && m_scraper->HasSettings())
      CONTROL_ENABLE(CONTROL_SCRAPER_SETTINGS);
    else
      CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogContentSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();

  FillContentTypes();

  if (m_content != CONTENT_NONE && !m_scraper)
  { // select the default scraper for this contenttype
    AddonPtr defaultScraper;
    if (CAddonMgr::Get()->GetDefaultScraper(defaultScraper, m_content))
    {
      m_scraper = boost::dynamic_pointer_cast<CScraper>(defaultScraper->Clone());
    }
  }

  if (m_scraper && m_scraper->HasSettings())
  {
    CONTROL_ENABLE(CONTROL_SCRAPER_SETTINGS);
  }
  else
    CONTROL_DISABLE(CONTROL_SCRAPER_SETTINGS);
}

void CGUIDialogContentSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_CONTENT_TYPE);
  g_graphicsContext.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_CONTENT_TYPE);

  if (m_content != CONTENT_ALBUMS) // none does not apply to music
  {
    msg2.SetLabel("<"+g_localizeStrings.Get(231)+">");
    msg2.SetParam1(0);
    g_graphicsContext.SendMessage(msg2);
  }

  if (m_scrapers.find(CONTENT_MOVIES) != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20342));
    msg2.SetParam1(1);
    g_graphicsContext.SendMessage(msg2);
    if (m_content == CONTENT_MOVIES)
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20342));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 1);
    }
  }

  if (m_scrapers.find(CONTENT_TVSHOWS) != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20343));
    msg2.SetParam1(2);
    g_graphicsContext.SendMessage(msg2);
    if (m_content == CONTENT_TVSHOWS)
    {
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 2);
    }
  }
  if (m_scrapers.find(CONTENT_MUSICVIDEOS) != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20389));
    msg2.SetParam1(3);
    g_graphicsContext.SendMessage(msg2);
    if (m_content == CONTENT_MUSICVIDEOS)
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20389));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 3);
    }
  }
  if (m_scrapers.find(CONTENT_ALBUMS) != m_scrapers.end())
  {
    msg2.SetLabel(m_strContent);
    msg2.SetParam1(4);
    g_graphicsContext.SendMessage(msg2);
    if (m_content == CONTENT_ALBUMS)
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,m_strContent);
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 3);
    }
  }
  SET_CONTROL_VISIBLE(CONTROL_CONTENT_TYPE);
  // now add them scrapers to the list control
  if (m_content == CONTENT_NONE)
  {
    CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
    OnMessage(msgReset);
    CONTROL_DISABLE(CONTROL_SCRAPER_LIST);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_SCRAPER_LIST);
    FillListControl();
  }

  OnSettingChanged(0);
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
      AddBool(3,20432,&m_bUpdate);
      break;
    }
  case CONTENT_MOVIES:
    {
      AddBool(1,20345,&m_bRunScan);
      AddBool(2,20330,&m_bUseDirNames);
      AddBool(3,20346,&m_bScanRecursive);
      AddBool(4,20383,&m_bSingleItem, m_bUseDirNames);
      AddBool(5,20432,&m_bUpdate);
      break;
    }
  case CONTENT_MUSICVIDEOS:
    {
      AddBool(1,20345,&m_bRunScan);
      AddBool(2,20346,&m_bScanRecursive);
      AddBool(3,20432,&m_bUpdate);
    }
  case CONTENT_ALBUMS:
    {
      AddBool(1,20345,&m_bRunScan);
    }
  default:
    break;
  }
}

void CGUIDialogContentSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  OnSettingChanged(setting);
}

void CGUIDialogContentSettings::OnSettingChanged(SettingInfo &setting)
{
  // check and update anything that needs it
  if (setting.id == 1 || setting.id == 2)
  {
    CreateSettings();
    UpdateSetting(1);
    UpdateSetting(2);
    UpdateSetting(3);
    UpdateSetting(4);
    UpdateSetting(5);
  }

  m_bNeedSave = true;
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
  FillContentTypes(CONTENT_MUSIC);
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

  if (!m_scraper)
  {
    if (!ADDON::CAddonMgr::Get()->GetDefaultScraper(m_scraper, m_content))
      return;
  }

  for (IVECADDONS iter=m_scrapers.find(m_content)->second.begin();iter!=m_scrapers.find(m_content)->second.end();++iter)
  {
    CFileItemPtr item(new CFileItem((*iter)->Name()));
    item->m_strPath = (*iter)->Path();
    item->SetThumbnailImage((*iter)->Icon());
    if (m_scraper && (*iter)->Path() == m_scraper->Path())
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
  if (currentItem == -1) return CFileItemPtr();
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

bool CGUIDialogContentSettings::Show(ADDON::CScraperPtr& scraper, bool& bRunScan, int iLabel)
{
  VIDEO::SScanSettings dummy;
  dummy.recurse = -1;
  dummy.parent_name = false;
  dummy.parent_name_root = false;
  dummy.noupdate = false;
  return Show(scraper,dummy,bRunScan,iLabel);
}

bool CGUIDialogContentSettings::Show(ADDON::CScraperPtr& scraper, VIDEO::SScanSettings& settings, bool& bRunScan, int iLabel)
{
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (!dialog) return false;

  if (scraper)
  {
    dialog->m_scraper = scraper;
    dialog->m_content = scraper->Content();
  }
  
  if (iLabel != -1) // to switch between albums and artists
    dialog->m_strContent = g_localizeStrings.Get(iLabel);

  dialog->m_bRunScan = bRunScan;
  dialog->m_bScanRecursive = (settings.recurse > 0 && !settings.parent_name) || (settings.recurse > 1 && settings.parent_name);
  dialog->m_bUseDirNames   = settings.parent_name;
  dialog->m_bExclude       = scraper && (scraper->Content() == CONTENT_NONE);
  dialog->m_bSingleItem    = settings.parent_name_root;
  dialog->m_bNeedSave = false;
  dialog->m_bUpdate = settings.noupdate;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    scraper = boost::dynamic_pointer_cast<CScraper>(dialog->m_scraper);
    if (!scraper)
      return true;

    scraper->m_pathContent = dialog->m_content;
    settings.noupdate = dialog->m_bUpdate;

    if (scraper->Content() == CONTENT_TVSHOWS)
    {
      settings.parent_name = dialog->m_bSingleItem;
      settings.parent_name_root = dialog->m_bSingleItem;
      settings.recurse = 0;

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper->Content() == CONTENT_MOVIES)
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

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper->Content() == CONTENT_MUSICVIDEOS)
    {
      settings.parent_name = false;
      settings.parent_name_root = false;
      settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper->Content() == CONTENT_ALBUMS)
    {
      bRunScan = dialog->m_bRunScan;
    }
    else if (dialog->m_bExclude)
    {
      bRunScan = false;
    }

    //TODO why load default with needsave == true?
    if (scraper->HasSettings())
    { // load default scraper settings
      scraper->LoadSettings();
      scraper->SaveFromDefault();
    }

    return true;
  }
  return false;
}

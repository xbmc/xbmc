#include "stdafx.h"
#include "GUIDialogContentSettings.h"
#include "Util.h"
#include "VideoDatabase.h"
#include "VideoInfoScanner.h"
#include "FileSystem/MultiPathDirectory.h"

#define CONTROL_CONTENT_TYPE        3
#define CONTROL_SCRAPER_LIST        4
#define CONTROL_START              30

using namespace DIRECTORY;

CGUIDialogContentSettings::CGUIDialogContentSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_CONTENT_SETTINGS, "DialogContentSettings.xml")
{
  m_bNeedSave = false;
}

CGUIDialogContentSettings::~CGUIDialogContentSettings(void)
{
}

bool CGUIDialogContentSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_scrapers.clear();
      m_vecItems.Clear();
      CGUIDialogSettings::OnMessage(message);
    }
    break;

  case GUI_MSG_ITEM_SELECT:
    {
      if (message.GetControlId() == CONTROL_SCRAPER_LIST)
      {
        if (!m_info.strContent.IsEmpty())
          m_info = m_scrapers[m_info.strContent][message.GetParam1()];
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
      case 0: m_info.strContent.Empty();
              m_info.strPath.Empty();
              m_info.strThumb.Empty();
              m_info.strTitle.Empty();
              OnSettingChanged(0);
              SetupPage();
              break;
      case 1: strLabel = g_localizeStrings.Get(20342);
              m_info = m_scrapers["movies"][0];
              CreateSettings();
              SetupPage();
              break;
      case 2: strLabel = g_localizeStrings.Get(20343);
              m_info = m_scrapers["tvshows"][0];
              CreateSettings();
              SetupPage();
              break;
      case 3: strLabel = g_localizeStrings.Get(20389);
              m_info = m_scrapers["musicvideos"][0];
              CreateSettings();
              SetupPage();
              break;
      }  
    }
    if (iControl == CONTROL_SCRAPER_LIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_SCRAPER_LIST);
      m_gWindowManager.SendMessage(msg);
      int iSelected = msg.GetParam1();
      m_info = m_scrapers[m_info.strContent][iSelected];
      FillListControl();
      SET_CONTROL_FOCUS(30,0);
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogContentSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();
  
  CFileItemList items;
  CDirectory::GetDirectory(_P("q:\\system\\scrapers\\video"),items,".xml",false);
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      TiXmlDocument doc;
      doc.LoadFile(items[i]->m_strPath);
      if (doc.RootElement())
      {
        const char* content = doc.RootElement()->Attribute("content");
        const char* name = doc.RootElement()->Attribute("name");
        const char* thumb = doc.RootElement()->Attribute("thumb");
        if (content && name)
        {
          SScraperInfo info;
          info.strTitle = name;
          info.strPath = CUtil::GetFileName(items[i]->m_strPath);
          if (thumb)
            info.strThumb = thumb;
          info.strContent = content;
          std::map<CStdString,std::vector<SScraperInfo> >::iterator iter=m_scrapers.find(content);
          if (iter != m_scrapers.end())
            iter->second.push_back(info);
          else
          {
            std::vector<SScraperInfo> vec;
            vec.push_back(info);
            m_scrapers.insert(std::make_pair<CStdString,std::vector<SScraperInfo> >(content,vec));
          }
        }
      }
    }
  }

  // now select the correct scraper
  if (!m_info.strContent.IsEmpty())
  {
    std::map<CStdString,std::vector<SScraperInfo> >::iterator iter = m_scrapers.find(m_info.strContent);
    if (iter != m_scrapers.end())
    {
      for (std::vector<SScraperInfo>::iterator iter2 = iter->second.begin();iter2 != iter->second.end();++iter2)
      {
        if (iter2->strPath == m_info.strPath)
        {
          m_info = *iter2;
          break;
        }
      }
    }
  }
}

void CGUIDialogContentSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_CONTENT_TYPE);
  g_graphicsContext.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_CONTENT_TYPE);

  msg2.SetLabel("<"+g_localizeStrings.Get(231)+">");
  msg2.SetParam1(0);
  g_graphicsContext.SendMessage(msg2);

  if (m_scrapers.find("movies") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20342));
    msg2.SetParam1(1);
    g_graphicsContext.SendMessage(msg2);
    if (m_info.strContent.Equals("movies"))
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20342));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 1);
    }
  }

  if (m_scrapers.find("tvshows") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20343));
    msg2.SetParam1(2);
    g_graphicsContext.SendMessage(msg2);
    if (m_info.strContent.Equals("tvshows"))
    {
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 2);
    }
  }
  if (m_scrapers.find("musicvideos") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20389));
    msg2.SetParam1(3);
    g_graphicsContext.SendMessage(msg2);
    if (m_info.strContent.Equals("musicvideos"))
    {
      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20389));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 3);
    }
  }
  SET_CONTROL_VISIBLE(CONTROL_CONTENT_TYPE);
  // now add them scrapers to the list control
  if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
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

  if (m_info.strContent.IsEmpty() || m_info.strContent.Equals("None"))
  {
    AddBool(1,20380,&m_bExclude);
  }
  if (m_info.strContent.Equals("movies"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20330,&m_bUseDirNames);
    AddBool(4,20383,&m_bSingleItem, m_bUseDirNames);
    AddBool(3,20346,&m_bScanRecursive);    
  }
  if (m_info.strContent.Equals("tvshows"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20379,&m_bSingleItem);
  }
  if (m_info.strContent.Equals("musicvideos"))
  {
    AddBool(1,20345,&m_bRunScan);
    AddBool(2,20346,&m_bScanRecursive);    
  }
}

void CGUIDialogContentSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it  
  if (setting.id == 1 || setting.id == 2)
  {
    CreateSettings();
    UpdateSetting(1);
    UpdateSetting(2);
    UpdateSetting(3);
    UpdateSetting(4);
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

void CGUIDialogContentSettings::FillListControl()
{
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
  OnMessage(msgReset); 
  int iIndex=0;
  m_vecItems.Clear();
  for (std::vector<SScraperInfo>::iterator iter=m_scrapers.find(m_info.strContent)->second.begin();iter!=m_scrapers.find(m_info.strContent)->second.end();++iter)
  {
    CFileItem* item = new CFileItem(iter->strTitle);
    item->m_strPath = iter->strPath;
    item->SetThumbnailImage(_P("Q:\\system\\scrapers\\video\\"+iter->strThumb));
    if (iter->strPath.Equals(m_info.strPath))
    {
      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SCRAPER_LIST, iIndex);
      OnMessage(msg2);
      item->Select(true);
    }
    m_vecItems.Add(item);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SCRAPER_LIST, 0, 0, (void*)item);
    OnMessage(msg);
    iIndex++;
  }
}

CFileItem* CGUIDialogContentSettings::GetCurrentListItem(int offset)
{
  int currentItem = -1;
  if( m_info.strContent.IsEmpty())
    return NULL;
  for (int i=0;i<m_vecItems.Size();++i )
  {
    if (m_vecItems[i]->IsSelected())
    {
      currentItem = i;
      break;
    }
  }
  if (currentItem == -1) return NULL;
  int item = (currentItem + offset) % m_vecItems.Size();
  if (item < 0) item += m_vecItems.Size();
  return m_vecItems[item];
}

bool CGUIDialogContentSettings::ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan)
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

bool CGUIDialogContentSettings::Show(SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan)
{
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (!dialog) return false;

  dialog->m_info = scraper;
  dialog->m_bRunScan = bRunScan;
  dialog->m_bScanRecursive = (settings.recurse > 0) && (!settings.parent_name && settings.recurse == 1);
  dialog->m_bUseDirNames   = settings.parent_name;
  dialog->m_bExclude       = scraper.strContent.Equals("None");
  dialog->m_bSingleItem    = settings.parent_name_root;
  dialog->m_bNeedSave = false;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    scraper = dialog->m_info;

    if (scraper.strContent.Equals("tvshows"))
    {
      settings.parent_name = dialog->m_bSingleItem;
      settings.parent_name_root = dialog->m_bSingleItem;
      settings.recurse = 0;
      
      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.Equals("movies"))
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
    else if (scraper.strContent.Equals("musicvideos"))
    {            
      settings.parent_name = false;
      settings.parent_name_root = false;
      settings.recurse = dialog->m_bScanRecursive ? INT_MAX : 0;

      bRunScan = dialog->m_bRunScan;
    }
    else if (scraper.strContent.IsEmpty() || scraper.strContent.Equals("None") )
    {
      if (dialog->m_bExclude)
        scraper.strContent = "None";
      else
        scraper.strContent = "";

      bRunScan = false;
    }
    
    return true;
  }
  return false;
}


#include "stdafx.h"
#include "GUIDialogContentSettings.h"
#include "Util.h"
#include "picture.h"

#define CONTROL_IMAGE               2
#define CONTROL_CONTENT_TYPE        1343
#define CONTROL_SCRAPER_LIST        4
#define CONTROL_START              30

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
      items.Clear();
      CGUIDialogSettings::OnMessage(message);
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

      CStdString strLabel;
      switch (iSelected)
      {
      case 0: m_info.strContent.Empty();
              m_info.strPath.Empty();
              m_info.strThumb.Empty();
              m_info.strTitle.Empty();
              ((CGUIImage*)GetControl(CONTROL_IMAGE))->SetFileName("");
              OnSettingChanged(0);
              SetupPage();
              break;
      case 1: strLabel = g_localizeStrings.Get(20342);
              m_info = m_scrapers.find("movies")->second[0];
              CreateSettings();
              SetupPage();
              break;
      case 2: strLabel = g_localizeStrings.Get(20343);
              m_info = m_scrapers["tvshows"][0];
              CreateSettings();
              SetupPage();
              break;
      }
      //SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,strLabel);
      CreateSettings();
      SetupPage();
    }
    break;
  }
  return CGUIDialogSettings::OnMessage(message);
}

void CGUIDialogContentSettings::OnWindowLoaded()
{
  CGUIDialogSettings::OnWindowLoaded();
  
  CFileItemList items;
  CDirectory::GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
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
  m_info.strContent = "movies";
  m_info.strPath = "imdb.xml";
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
          m_info.strContent = iter->first;
          break;
        }
      }
    }
  }
}

void CGUIDialogContentSettings::SetupPage()
{
  CGUIDialogSettings::SetupPage();
  CGUIImage *pImage = (CGUIImage*)GetControl(CONTROL_IMAGE);
  pImage->SetFileName("q:\\system\\scrapers\\video\\"+m_info.strThumb);

  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_CONTENT_TYPE);
  g_graphicsContext.SendMessage(msg);
  CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_CONTENT_TYPE);

  msg2.SetLabel(g_localizeStrings.Get(20337));
  msg2.SetParam1(0);
  g_graphicsContext.SendMessage(msg2);

  if (m_scrapers.find("movies") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20342));
    msg2.SetParam1(1);
    g_graphicsContext.SendMessage(msg2);
    if (m_info.strContent.Equals("movies"))
    {
//      SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20342));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 0);
    }
  }

  if (m_scrapers.find("tvshows") != m_scrapers.end())
  {
    msg2.SetLabel(g_localizeStrings.Get(20343));
    msg2.SetParam1(2);
    g_graphicsContext.SendMessage(msg2);
    if (m_info.strContent.Equals("tvshows"))
    {
  //    SET_CONTROL_LABEL(CONTROL_CONTENT_TYPE,g_localizeStrings.Get(20343));
      CONTROL_SELECT_ITEM(CONTROL_CONTENT_TYPE, 1);
    }
  }
  SET_CONTROL_VISIBLE(CONTROL_CONTENT_TYPE);
  // now add them scrapers to the list control
  FillListControl();
}

void CGUIDialogContentSettings::CreateSettings()
{
  // clear out any old settings
  m_settings.clear();

  AddBool(1,20345,&m_bRunScan);  
  AddBool(2,20336,&m_bScanSeveral,m_bRunScan);
  AddBool(3,20346,&m_bScanRecursive,m_bRunScan);

  if (m_info.strContent.Equals("movies"))
  {
    AddBool(4,20330,&m_bUseDirNames,m_bRunScan);
  }
}

void CGUIDialogContentSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;
  SettingInfo &setting = m_settings.at(num);
  // check and update anything that needs it
  if (setting.id == 1)
  {
    if (m_info.strContent.IsEmpty())
    {
      m_settings[0].enabled = m_settings[1].enabled = m_settings[2].enabled = false;
      if (m_settings.size() > 3)
        m_settings[3].enabled = false;
      
      UpdateSetting(1);
      UpdateSetting(2);
      UpdateSetting(3);
      UpdateSetting(4);
    }
    if (m_info.strContent.Equals("movies"))
    {
      m_settings[1].enabled = *((bool*)setting.data);
      m_settings[2].enabled = *((bool*)setting.data);
      m_settings[3].enabled = *((bool*)setting.data);
      UpdateSetting(2);
      UpdateSetting(3);
      UpdateSetting(4);
    }
    if (m_info.strContent.Equals("tvshows"))
    {
      m_settings[1].enabled = m_settings[2].enabled = *((bool*)setting.data);
      UpdateSetting(2);
      UpdateSetting(3);
    }
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
}

void CGUIDialogContentSettings::FillListControl()
{
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SCRAPER_LIST);
  OnMessage(msgReset); 
  int iIndex=0;
  items.Clear();
  for (std::vector<SScraperInfo>::iterator iter=m_scrapers.find(m_info.strContent)->second.begin();iter!=m_scrapers.find(m_info.strContent)->second.end();++iter)
  {
    CFileItem* item = new CFileItem(iter->strTitle);
    item->m_strPath = iter->strPath;
    items.Add(item);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SCRAPER_LIST, 0, 0, (void*)item);
    OnMessage(msg);
    if (iter->strTitle.Equals(m_info.strTitle))
    {
      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SCRAPER_LIST, iIndex);
      OnMessage(msg2);
      CGUIImage *pImage = (CGUIImage*)GetControl(2);
      pImage->SetFileName("q:\\system\\scrapers\\video\\"+m_info.strThumb);      
    }
    iIndex++;
  }
}

bool CGUIDialogContentSettings::ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, bool& bRunScan, bool& bScanRecursive, bool& bScanSeveral, bool &bUseDirNames)
{
  CGUIDialogContentSettings *dialog = (CGUIDialogContentSettings *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
  if (!dialog) return false;

  dialog->m_info = scraper;
  dialog->m_bRunScan = false;
  dialog->m_bScanRecursive = true;
  dialog->m_bScanSeveral = false;
  dialog->m_bUseDirNames = false;
  dialog->m_bNeedSave = false;
  dialog->DoModal();
  if (dialog->m_bNeedSave)
  {
    scraper = dialog->m_info;
    bRunScan = dialog->m_bRunScan;
    bScanRecursive = dialog->m_bScanRecursive;
    bScanSeveral = dialog->m_bScanSeveral;
    bUseDirNames = dialog->m_bUseDirNames;
    return true;
  }

  return !dialog->m_bNeedSave;
}
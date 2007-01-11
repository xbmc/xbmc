#pragma once

#include "GUIDialogSettings.h"
#include "utils/IMDB.h"

class CGUIDialogContentSettings : public CGUIDialogSettings
{
public:
  CGUIDialogContentSettings(void);
  virtual ~CGUIDialogContentSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, bool& bRunScan, bool& bScanRecursive, bool& bScanSeveral, bool& bUseDirNames);
protected:
  virtual void OnCancel();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void SetupPage();
  virtual void CreateSettings();
  void FillListControl();
  void OnSettingChanged(unsigned int setting);
  
  bool m_bNeedSave;

  bool m_bRunScan;
  bool m_bScanRecursive;
  bool m_bScanSeveral;
  bool m_bUseDirNames;
  std::map<CStdString,std::vector<SScraperInfo> > m_scrapers; // key = content type
  CFileItemList items;

  SScraperInfo m_info;
};
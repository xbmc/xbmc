#pragma once

#include "GUIDialogSettings.h"
#include "utils/IMDB.h"

namespace VIDEO {
struct SScanSettings;
}

class CGUIDialogContentSettings : public CGUIDialogSettings
{
public:
  CGUIDialogContentSettings(void);
  virtual ~CGUIDialogContentSettings(void);
  virtual bool OnMessage(CGUIMessage &message);

  static bool Show(SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan);
  static bool ShowForDirectory(const CStdString& strDirectory, SScraperInfo& scraper, VIDEO::SScanSettings& settings, bool& bRunScan);
  virtual bool HasListItems() const { return true; };
  virtual CFileItem* GetCurrentListItem(int offset = 0);
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
  bool m_bUseDirNames;
  bool m_bSingleItem;
  bool m_bExclude;
  std::map<CStdString,std::vector<SScraperInfo> > m_scrapers; // key = content type
  CFileItemList m_vecItems;

  SScraperInfo m_info;
};
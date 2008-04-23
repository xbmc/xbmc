#ifndef SCRAPERSETTINGS_H_
#define SCRAPERSETTINGS_H_

#include "PluginSettings.h"

class CScraperUrl;

class CScraperSettings : public CBasicSettings
{
public:
  CScraperSettings();
  virtual ~CScraperSettings();
  bool LoadUserXML(const CStdString& strXML);
  bool LoadSettingsXML(const CStdString& strScraper, const CStdString& strFunction="GetSettings", const CScraperUrl* url=NULL);
  bool Load(const CStdString& strSettings, const CStdString& strSaved);
  CStdString GetSettings() const;
};

struct SScraperInfo
{
  CStdString strTitle;
  CStdString strPath;
  CStdString strThumb;
  CStdString strContent; // dupe, whatever
  CScraperSettings settings;
};

#endif

